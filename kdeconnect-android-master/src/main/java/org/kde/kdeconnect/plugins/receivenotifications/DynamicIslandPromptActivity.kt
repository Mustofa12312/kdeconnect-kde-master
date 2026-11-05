/*
 * SPDX-FileCopyrightText: 2026 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

package org.kde.kdeconnect.plugins.receivenotifications

import android.app.NotificationManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.VibrationEffect
import android.os.Vibrator
import android.util.Log
import android.view.Gravity
import android.view.WindowManager
import androidx.activity.compose.setContent
import androidx.appcompat.app.AppCompatActivity
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import androidx.core.content.getSystemService
import kotlinx.coroutines.delay
import org.kde.kdeconnect.KdeConnect
import org.kde.kdeconnect.NetworkPacket

data class PromptActionItem(val key: String, val label: String)

class DynamicIslandPromptActivity : AppCompatActivity() {

    private var deviceId: String = ""
    private var notificationId: String = ""
    private val handler = Handler(Looper.getMainLooper())
    private val dismissTimeoutRunnable = Runnable { finish() }

    private val dismissReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == NotificationActionReceiver.ACTION_DISMISS_DYNAMIC_ISLAND) {
                val id = intent.getStringExtra(NotificationActionReceiver.EXTRA_NOTIFICATION_ID)
                if (id == null || id == notificationId) {
                    finish()
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Configure window for Apple-style top Dynamic Island pill
        setupWindow()

        deviceId = intent.getStringExtra(EXTRA_DEVICE_ID) ?: ""
        notificationId = intent.getStringExtra(EXTRA_NOTIFICATION_ID) ?: ""
        val appName = intent.getStringExtra(EXTRA_TITLE) ?: "Authorization Request"
        val summary = intent.getStringExtra(EXTRA_SUMMARY) ?: ""
        val body = intent.getStringExtra(EXTRA_BODY) ?: ""
        val rawActions = intent.getStringArrayListExtra(EXTRA_ACTIONS) ?: arrayListOf()

        val actionItems = parseActions(rawActions)

        // Register dismiss receiver
        val filter = IntentFilter(NotificationActionReceiver.ACTION_DISMISS_DYNAMIC_ISLAND)
        ContextCompat.registerReceiver(
            this,
            dismissReceiver,
            filter,
            ContextCompat.RECEIVER_NOT_EXPORTED
        )

        // Auto dismiss after 60s
        handler.postDelayed(dismissTimeoutRunnable, 60000L)

        setContent {
            DynamicIslandContent(
                appName = appName,
                summary = summary,
                body = body,
                actions = actionItems,
                onActionSelected = { actionKey ->
                    handleActionSelected(actionKey)
                },
                onDismiss = {
                    finish()
                }
            )
        }
    }

    private fun setupWindow() {
        val wlp = window.attributes
        wlp.gravity = Gravity.TOP or Gravity.CENTER_HORIZONTAL
        wlp.y = 28 // position immediately under front camera cutout
        wlp.width = WindowManager.LayoutParams.MATCH_PARENT
        wlp.height = WindowManager.LayoutParams.WRAP_CONTENT
        wlp.dimAmount = 0.25f
        wlp.flags = WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN or
                WindowManager.LayoutParams.FLAG_DIM_BEHIND
        window.attributes = wlp
    }

    private fun parseActions(raw: List<String>): List<PromptActionItem> {
        val result = mutableListOf<PromptActionItem>()
        if (raw.isEmpty()) {
            result.add(PromptActionItem("1", "Allow Once"))
            result.add(PromptActionItem("2", "Always Allow"))
            result.add(PromptActionItem("3", "Deny"))
            return result
        }

        if (raw.size % 2 == 0) {
            for (i in raw.indices step 2) {
                result.add(PromptActionItem(raw[i], raw[i + 1]))
            }
        } else {
            for (item in raw) {
                if (item.contains(":")) {
                    val parts = item.split(":", limit = 2)
                    result.add(PromptActionItem(parts[0].trim(), parts[1].trim()))
                } else {
                    result.add(PromptActionItem(item, item))
                }
            }
        }
        return result
    }

    private fun handleActionSelected(actionKey: String) {
        // Haptic feedback
        try {
            val vibrator = getSystemService<Vibrator>()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                vibrator?.vibrate(VibrationEffect.createPredefined(VibrationEffect.EFFECT_CLICK))
            } else {
                @Suppress("DEPRECATION")
                vibrator?.vibrate(35)
            }
        } catch (e: Exception) {
            Log.w(TAG, "Failed haptic vibration", e)
        }

        // Send packet to desktop
        val device = KdeConnect.getInstance().getDevice(deviceId)
        if (device != null) {
            val np = NetworkPacket(ReceiveNotificationsPlugin.PACKET_TYPE_NOTIFICATION_ACTION)
            np["id"] = notificationId
            np["action"] = actionKey
            device.sendPacket(np)
            Log.i(TAG, "Action $actionKey sent to device $deviceId")
        }

        // Cancel notification
        val nm = getSystemService<NotificationManager>()
        val intId = try { notificationId.toInt() } catch (e: Exception) { 0 }
        nm?.cancel("kdeconnectId:$notificationId", intId)

        finish()
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacks(dismissTimeoutRunnable)
        try {
            unregisterReceiver(dismissReceiver)
        } catch (e: Exception) {
            // Receiver already unregistered
        }
    }

    companion object {
        private const val TAG = "KDE/DynamicIslandPrompt"

        const val EXTRA_DEVICE_ID = "EXTRA_DEVICE_ID"
        const val EXTRA_NOTIFICATION_ID = "EXTRA_NOTIFICATION_ID"
        const val EXTRA_TITLE = "EXTRA_TITLE"
        const val EXTRA_SUMMARY = "EXTRA_SUMMARY"
        const val EXTRA_BODY = "EXTRA_BODY"
        const val EXTRA_ACTIONS = "EXTRA_ACTIONS"

        fun getIntent(
            context: Context,
            deviceId: String,
            notificationId: String,
            title: String,
            summary: String,
            body: String,
            actions: ArrayList<String>
        ): Intent {
            return Intent(context, DynamicIslandPromptActivity::class.java).apply {
                putExtra(EXTRA_DEVICE_ID, deviceId)
                putExtra(EXTRA_NOTIFICATION_ID, notificationId)
                putExtra(EXTRA_TITLE, title)
                putExtra(EXTRA_SUMMARY, summary)
                putExtra(EXTRA_BODY, body)
                putStringArrayListExtra(EXTRA_ACTIONS, actions)
                flags = Intent.FLAG_ACTIVITY_NEW_TASK or
                        Intent.FLAG_ACTIVITY_CLEAR_TOP or
                        Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS
            }
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun DynamicIslandContent(
    appName: String,
    summary: String,
    body: String,
    actions: List<PromptActionItem>,
    onActionSelected: (String) -> Unit,
    onDismiss: () -> Unit
) {
    var visible by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        visible = true
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(horizontal = 14.dp, vertical = 6.dp),
        contentAlignment = Alignment.TopCenter
    ) {
        AnimatedVisibility(
            visible = visible,
            enter = slideInVertically(initialOffsetY = { -it }) + fadeIn(),
            exit = slideOutVertically(targetOffsetY = { -it }) + fadeOut()
        ) {
            // Apple Dynamic Island Pill Card
            Surface(
                modifier = Modifier
                    .fillMaxWidth()
                    .shadow(elevation = 16.dp, shape = RoundedCornerShape(26.dp), spotColor = Color(0x66000000))
                    .clip(RoundedCornerShape(26.dp))
                    .border(width = 1.dp, color = Color(0x33FFFFFF), shape = RoundedCornerShape(26.dp)),
                color = Color(0xF00D0D11), // Pitch black iOS translucency
                contentColor = Color.White
            ) {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 18.dp, vertical = 14.dp)
                ) {
                    // Header Row: App icon, Title, Pulsing green dot, Close button
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        // Glowing Apple Icon Badge
                        Box(
                            modifier = Modifier
                                .size(28.dp)
                                .clip(CircleShape)
                                .background(Color(0xFF0A84FF)),
                            contentAlignment = Alignment.Center
                        ) {
                            Text(
                                text = ">_",
                                color = Color.White,
                                fontSize = 11.sp,
                                fontWeight = FontWeight.Bold,
                                fontFamily = FontFamily.Monospace
                            )
                        }

                        Spacer(modifier = Modifier.width(10.dp))

                        Column(modifier = Modifier.weight(1f)) {
                            Text(
                                text = appName,
                                color = Color.White,
                                fontSize = 13.sp,
                                fontWeight = FontWeight.SemiBold,
                                maxLines = 1,
                                overflow = TextOverflow.Ellipsis
                            )
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                // Green active indicator dot
                                Box(
                                    modifier = Modifier
                                        .size(6.dp)
                                        .clip(CircleShape)
                                        .background(Color(0xFF30D158))
                                )
                                Spacer(modifier = Modifier.width(5.dp))
                                Text(
                                    text = "Remote Request",
                                    color = Color(0xFF8E8E93),
                                    fontSize = 11.sp
                                )
                            }
                        }

                        // Close button
                        IconButton(
                            onClick = onDismiss,
                            modifier = Modifier.size(24.dp)
                        ) {
                            Icon(
                                imageVector = Icons.Default.Close,
                                contentDescription = "Close",
                                tint = Color(0xFF8E8E93),
                                modifier = Modifier.size(16.dp)
                            )
                        }
                    }

                    // Summary title if provided
                    if (summary.isNotBlank()) {
                        Spacer(modifier = Modifier.height(10.dp))
                        Text(
                            text = summary,
                            color = Color(0xFFE5E5EA),
                            fontSize = 13.sp,
                            fontWeight = FontWeight.Medium,
                            maxLines = 2,
                            overflow = TextOverflow.Ellipsis
                        )
                    }

                    // Monospace Command / Details Capsule
                    if (body.isNotBlank()) {
                        Spacer(modifier = Modifier.height(8.dp))
                        Box(
                            modifier = Modifier
                                .fillMaxWidth()
                                .heightIn(max = 140.dp)
                                .clip(RoundedCornerShape(12.dp))
                                .background(Color(0xFF1C1C1E))
                                .border(0.5.dp, Color(0x26FFFFFF), RoundedCornerShape(12.dp))
                                .verticalScroll(rememberScrollState())
                                .padding(horizontal = 12.dp, vertical = 8.dp)
                        ) {
                            Text(
                                text = body,
                                color = Color(0xFF30D158),
                                fontSize = 12.sp,
                                fontFamily = FontFamily.Monospace,
                                lineHeight = 16.sp
                            )
                        }
                    }

                    Spacer(modifier = Modifier.height(14.dp))

                    // Apple Style Action Buttons Row
                    FlowRow(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        for (item in actions) {
                            val isAllow = item.key.contains("1") ||
                                    item.label.contains("allow", ignoreCase = true) ||
                                    item.label.contains("yes", ignoreCase = true)
                            val isDeny = item.key.contains("3") ||
                                    item.key.contains("4") ||
                                    item.label.contains("deny", ignoreCase = true) ||
                                    item.label.contains("no", ignoreCase = true)
                            val isAlways = item.key.contains("2") ||
                                    item.label.contains("always", ignoreCase = true)

                            val btnBg = when {
                                isAlways -> Color(0xFF2C2C2E) // Apple Secondary Gray
                                isDeny -> Color(0xFF3A1818) // Apple Subtle Red surface
                                isAllow -> Color(0xFF30D158) // Apple Vibrant Green
                                else -> Color(0xFF007AFF) // Apple Blue
                            }

                            val btnTextColor = when {
                                isAllow && !isAlways -> Color.Black
                                isDeny -> Color(0xFFFF453A)
                                else -> Color.White
                            }

                            val borderModifier = if (isDeny) {
                                Modifier.border(1.dp, Color(0x66FF453A), RoundedCornerShape(20.dp))
                            } else if (isAlways) {
                                Modifier.border(0.8.dp, Color(0x33FFFFFF), RoundedCornerShape(20.dp))
                            } else {
                                Modifier
                            }

                            Button(
                                onClick = { onActionSelected(item.key) },
                                modifier = Modifier
                                    .height(38.dp)
                                    .then(borderModifier),
                                shape = RoundedCornerShape(20.dp),
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = btnBg,
                                    contentColor = btnTextColor
                                ),
                                contentPadding = androidx.compose.foundation.layout.PaddingValues(
                                    horizontal = 16.dp,
                                    vertical = 4.dp
                                )
                            ) {
                                Text(
                                    text = item.label,
                                    fontSize = 12.sp,
                                    fontWeight = FontWeight.SemiBold
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}

