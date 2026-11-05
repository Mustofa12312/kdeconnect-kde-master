/*
 * SPDX-FileCopyrightText: 2014 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
package org.kde.kdeconnect.plugins.receivenotifications

import android.Manifest
import android.app.Notification
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Build
import android.os.PowerManager
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.content.getSystemService
import androidx.core.graphics.scale
import org.kde.kdeconnect.NetworkPacket
import org.kde.kdeconnect.helpers.NotificationHelper
import org.kde.kdeconnect.plugins.Plugin
import org.kde.kdeconnect.plugins.PluginFactory.LoadablePlugin
import org.kde.kdeconnect.ui.MainActivity
import org.kde.kdeconnect_tp.R

@LoadablePlugin
class ReceiveNotificationsPlugin : Plugin() {
    override val displayName: String
        get() = context.resources.getString(R.string.pref_plugin_receive_notifications)

    override val description: String
        get() = context.resources.getString(R.string.pref_plugin_receive_notifications_desc)

    override val isEnabledByDefault: Boolean = true

    override fun onCreate() {
        // request all existing notifications
        val np = NetworkPacket(PACKET_TYPE_NOTIFICATION_REQUEST)
        np["request"] = true
        device.sendPacket(np)
    }

    override fun onPacketReceived(np: NetworkPacket): Boolean {
        if ("ticker" !in np || "appName" !in np || "id" !in np) {
            Log.e("NotificationsPlugin", "Received notification packet lacks properties")
            return true
        }

        val id = np.getString("id")
        val appName = np.getString("appName")
        val ticker = np.getString("ticker")
        val title = np.getString("title", appName)
        val text = np.getString("text", ticker)
        val actionsList = np.getStringList("actions")
        val isPrompt = np.getBoolean("isPrompt", false) || (!actionsList.isNullOrEmpty())

        val resultPendingIntent = PendingIntent.getActivity(
            context,
            0,
            Intent(context, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        var largeIcon: Bitmap? = null
        val payload = np.payload
        if (payload != null && payload.payloadSize != 0L) {
            val width = context.resources.getDimensionPixelSize(android.R.dimen.notification_large_icon_width)
            val height = context.resources.getDimensionPixelSize(android.R.dimen.notification_large_icon_height)
            val input = payload.inputStream
            largeIcon = BitmapFactory.decodeStream(input)
            payload.close()

            if (largeIcon != null) {
                if (largeIcon.width > width || largeIcon.height > height) {
                    largeIcon = largeIcon.scale(width, height, false)
                }
            }
        }

        val notificationManager = context.getSystemService<NotificationManager>() ?: return true

        val channelId = if (isPrompt) {
            NotificationHelper.Channels.HIGHPRIORITY
        } else {
            NotificationHelper.Channels.RECEIVENOTIFICATION
        }

        val notiBuilder = NotificationCompat.Builder(context, channelId)
            .setContentTitle(appName)
            .setContentText(ticker)
            .setContentIntent(resultPendingIntent)
            .setTicker(ticker)
            .setSmallIcon(R.drawable.ic_notification)
            .setLargeIcon(largeIcon)
            .setAutoCancel(true)
            .setLocalOnly(true)
            .setDefaults(Notification.DEFAULT_ALL)
            .setSilent(np.getBoolean("silent", false))
            .setStyle(NotificationCompat.BigTextStyle().bigText(ticker))

        if (isPrompt) {
            notiBuilder.setColor(0xFF007AFF.toInt()) // Apple iOS Blue Accent
            notiBuilder.priority = NotificationCompat.PRIORITY_MAX
            notiBuilder.setCategory(NotificationCompat.CATEGORY_REMINDER)

            val actionsArrayList = ArrayList(actionsList ?: emptyList())
            val promptIntent = DynamicIslandPromptActivity.getIntent(
                context = context,
                deviceId = device.deviceId,
                notificationId = id,
                title = appName,
                summary = title,
                body = text,
                actions = actionsArrayList
            )

            val promptPendingIntent = PendingIntent.getActivity(
                context,
                id.hashCode(),
                promptIntent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
            notiBuilder.setFullScreenIntent(promptPendingIntent, true)

            // Add notification action buttons for notification shade
            var reqCode = id.hashCode() + 10
            if (actionsList != null && actionsList.isNotEmpty()) {
                if (actionsList.size % 2 == 0) {
                    for (i in actionsList.indices step 2) {
                        val actKey = actionsList[i]
                        val actLabel = actionsList[i + 1]
                        val pIntent = NotificationActionReceiver.createPendingIntent(
                            context, device.deviceId, id, actKey, reqCode++
                        )
                        notiBuilder.addAction(0, actLabel, pIntent)
                    }
                } else {
                    for (act in actionsList) {
                        val actKey = if (act.contains(":")) act.split(":", limit = 2)[0].trim() else act
                        val actLabel = if (act.contains(":")) act.split(":", limit = 2)[1].trim() else act
                        val pIntent = NotificationActionReceiver.createPendingIntent(
                            context, device.deviceId, id, actKey, reqCode++
                        )
                        notiBuilder.addAction(0, actLabel, pIntent)
                    }
                }
            } else {
                // Default authorization choices
                notiBuilder.addAction(0, "Allow Once", NotificationActionReceiver.createPendingIntent(context, device.deviceId, id, "1", reqCode++))
                notiBuilder.addAction(0, "Always Allow", NotificationActionReceiver.createPendingIntent(context, device.deviceId, id, "2", reqCode++))
                notiBuilder.addAction(0, "Deny", NotificationActionReceiver.createPendingIntent(context, device.deviceId, id, "3", reqCode++))
            }

            // If phone screen is currently ON/in-use, launch Dynamic Island prompt directly
            val powerManager = context.getSystemService<PowerManager>()
            val isInteractive = powerManager?.isInteractive == true
            if (isInteractive) {
                try {
                    context.startActivity(promptIntent)
                } catch (e: Exception) {
                    Log.w("NotificationsPlugin", "Failed to start DynamicIslandPromptActivity directly", e)
                }
            }
        }

        val intId = try { id.toInt() } catch (e: NumberFormatException) { 0 }
        notificationManager.notify("kdeconnectId:${id}", intId, notiBuilder.build())

        return true
    }

    override val supportedPacketTypes: Array<String> = arrayOf(PACKET_TYPE_NOTIFICATION)

    override val outgoingPacketTypes: Array<String> = arrayOf(
        PACKET_TYPE_NOTIFICATION_REQUEST,
        PACKET_TYPE_NOTIFICATION_ACTION
    )

    override val requiredPermissions: Array<String> = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        arrayOf(Manifest.permission.POST_NOTIFICATIONS)
    } else {
        arrayOf()
    }

    override val permissionExplanation: Int = R.string.receive_notifications_permission_explanation

    companion object {
        const val PACKET_TYPE_NOTIFICATION = "kdeconnect.notification"
        const val PACKET_TYPE_NOTIFICATION_REQUEST = "kdeconnect.notification.request"
        const val PACKET_TYPE_NOTIFICATION_ACTION = "kdeconnect.notification.action"
    }
}
