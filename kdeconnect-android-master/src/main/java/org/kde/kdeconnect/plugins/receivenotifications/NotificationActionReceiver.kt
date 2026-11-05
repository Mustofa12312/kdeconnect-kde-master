/*
 * SPDX-FileCopyrightText: 2026 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

package org.kde.kdeconnect.plugins.receivenotifications

import android.app.NotificationManager
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.VibrationEffect
import android.os.Vibrator
import android.util.Log
import androidx.core.content.getSystemService
import org.kde.kdeconnect.KdeConnect
import org.kde.kdeconnect.NetworkPacket

class NotificationActionReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != ACTION_NOTIFICATION_ACTION) {
            return
        }

        val deviceId = intent.getStringExtra(EXTRA_DEVICE_ID) ?: return
        val notifId = intent.getStringExtra(EXTRA_NOTIFICATION_ID) ?: return
        val actionKey = intent.getStringExtra(EXTRA_ACTION_KEY) ?: return

        Log.d(TAG, "Notification action received: device=$deviceId id=$notifId action=$actionKey")

        // Tactile haptic feedback
        try {
            val vibrator = context.getSystemService<Vibrator>()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                vibrator?.vibrate(VibrationEffect.createPredefined(VibrationEffect.EFFECT_CLICK))
            } else {
                @Suppress("DEPRECATION")
                vibrator?.vibrate(35)
            }
        } catch (e: Exception) {
            Log.w(TAG, "Failed to vibrate", e)
        }

        // Cancel the system notification
        val nm = context.getSystemService<NotificationManager>()
        val intId = try { notifId.toInt() } catch (e: Exception) { 0 }
        nm?.cancel("kdeconnectId:$notifId", intId)

        // Dismiss floating dynamic island prompt if active
        val dismissIntent = Intent(ACTION_DISMISS_DYNAMIC_ISLAND).apply {
            setPackage(context.packageName)
            putExtra(EXTRA_NOTIFICATION_ID, notifId)
        }
        context.sendBroadcast(dismissIntent)

        // Send action packet back to desktop
        val device = KdeConnect.getInstance().getDevice(deviceId)
        if (device != null) {
            val np = NetworkPacket(ReceiveNotificationsPlugin.PACKET_TYPE_NOTIFICATION_ACTION)
            np["id"] = notifId
            np["action"] = actionKey
            device.sendPacket(np)
            Log.i(TAG, "Sent notification action to desktop device: $actionKey")
        } else {
            Log.w(TAG, "Device $deviceId not found to send notification action")
        }
    }

    companion object {
        const val ACTION_NOTIFICATION_ACTION = "org.kde.kdeconnect.action.NOTIFICATION_ACTION"
        const val ACTION_DISMISS_DYNAMIC_ISLAND = "org.kde.kdeconnect.action.DISMISS_DYNAMIC_ISLAND"

        const val EXTRA_DEVICE_ID = "EXTRA_DEVICE_ID"
        const val EXTRA_NOTIFICATION_ID = "EXTRA_NOTIFICATION_ID"
        const val EXTRA_ACTION_KEY = "EXTRA_ACTION_KEY"

        private const val TAG = "KDE/NotifActionReceiver"

        fun createPendingIntent(
            context: Context,
            deviceId: String,
            notificationId: String,
            actionKey: String,
            requestCode: Int
        ): PendingIntent {
            val intent = Intent(context, NotificationActionReceiver::class.java).apply {
                action = ACTION_NOTIFICATION_ACTION
                putExtra(EXTRA_DEVICE_ID, deviceId)
                putExtra(EXTRA_NOTIFICATION_ID, notificationId)
                putExtra(EXTRA_ACTION_KEY, actionKey)
            }
            return PendingIntent.getBroadcast(
                context,
                requestCode,
                intent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
        }
    }
}

