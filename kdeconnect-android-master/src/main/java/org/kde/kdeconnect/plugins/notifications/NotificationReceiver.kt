/*
 * SPDX-FileCopyrightText: 2014 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

package org.kde.kdeconnect.plugins.notifications

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.os.Build
import android.provider.Settings
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import androidx.core.app.NotificationManagerCompat
import java.util.concurrent.locks.Lock
import java.util.concurrent.locks.ReentrantLock

class NotificationReceiver : NotificationListenerService() {

    var isConnected: Boolean = false
        private set

    interface NotificationListener {
        fun onNotificationPosted(statusBarNotification: StatusBarNotification)
        fun onNotificationRemoved(statusBarNotification: StatusBarNotification)
        fun onListenerConnected(service: NotificationReceiver)
    }

    private val listeners = mutableListOf<NotificationListener>()

    fun addListener(listener: NotificationListener) {
        synchronized(listeners) {
            if (!listeners.contains(listener)) {
                listeners.add(listener)
            }
        }

        if (isConnected) {
            listener.onListenerConnected(this)
        } else {
            requestRebindSafe()
        }
    }

    fun removeListener(listener: NotificationListener) {
        val shouldUnbind: Boolean
        synchronized(listeners) {
            listeners.remove(listener)
            shouldUnbind = listeners.isEmpty()
        }
        if (shouldUnbind) {
            requestUnbindSafe()
        }
    }

    private fun requestRebindSafe() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N || !hasReadNotificationsPermission(this)) {
            return
        }

        try {
            requestRebind(ComponentName(this, NotificationReceiver::class.java))
        } catch (_: Exception) {
            // Notification access may have been revoked since the permission check.
        }
    }

    private fun requestUnbindSafe() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
            return
        }

        isConnected = false
        if (instance == this) {
            instance = null
        }
        try {
            super.requestUnbind()
        } catch (_: Exception) {
            // The system may already have unbound the listener.
        }
    }

    override fun onNotificationPosted(statusBarNotification: StatusBarNotification) {
        val currentListeners: List<NotificationListener>
        synchronized(listeners) {
            currentListeners = listeners.toList()
        }
        for (listener in currentListeners) {
            listener.onNotificationPosted(statusBarNotification)
        }
    }

    override fun onNotificationRemoved(statusBarNotification: StatusBarNotification) {
        val currentListeners: List<NotificationListener>
        synchronized(listeners) {
            currentListeners = listeners.toList()
        }
        for (listener in currentListeners) {
            listener.onNotificationRemoved(statusBarNotification)
        }
    }

    override fun onListenerConnected() {
        super.onListenerConnected()
        instance = this
        isConnected = true
        drainCallbacks()

        val currentListeners: List<NotificationListener>
        synchronized(listeners) {
            currentListeners = listeners.toList()
        }
        for (listener in currentListeners) {
            listener.onListenerConnected(this)
        }
    }

    override fun onListenerDisconnected() {
        super.onListenerDisconnected()
        isConnected = false
        if (instance == this) {
            instance = null
        }

        val hasListeners: Boolean
        synchronized(listeners) {
            hasListeners = listeners.isNotEmpty()
        }
        if (hasListeners) {
            requestRebindSafe()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        isConnected = false
        if (instance == this) {
            instance = null
        }
    }

    private fun drainCallbacks() {
        val pending: List<InstanceCallback>
        mutex.lock()
        try {
            pending = callbacks.toList()
            callbacks.clear()
        } finally {
            mutex.unlock()
        }
        for (c in pending) {
            try {
                c.onServiceStart(this)
            } catch (e: Exception) {
                Log.e("NotificationReceiver", "Error in onServiceStart", e)
            }
        }
    }

    fun interface InstanceCallback {
        fun onServiceStart(service: NotificationReceiver)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        instance = this
        drainCallbacks()
        return START_STICKY
    }

    companion object {
        @Volatile
        var instance: NotificationReceiver? = null
            private set

        fun hasReadNotificationsPermission(context: Context): Boolean {
            try {
                val enabledPackages = NotificationManagerCompat.getEnabledListenerPackages(context)
                if (enabledPackages.contains(context.packageName)) {
                    return true
                }
            } catch (_: Exception) {}

            val notificationListenerList =
                Settings.Secure.getString(context.contentResolver, "enabled_notification_listeners")
                    ?: return false
            val thisComponentName = ComponentName(context, NotificationReceiver::class.java).flattenToString()
            return notificationListenerList.contains(thisComponentName) || notificationListenerList.contains(context.packageName)
        }

        private val callbacks = ArrayList<InstanceCallback>()
        private val mutex: Lock = ReentrantLock(true)

        fun Start(c: Context) {
            RunCommand(c, null)
        }

        fun RunCommand(c: Context, callback: InstanceCallback?) {
            val inst = instance
            if (inst != null && inst.isConnected) {
                callback?.onServiceStart(inst)
                return
            }

            if (callback != null) {
                mutex.lock()
                try {
                    callbacks.add(callback)
                } finally {
                    mutex.unlock()
                }
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N && hasReadNotificationsPermission(c)) {
                try {
                    instance?.requestRebindSafe()
                } catch (_: Exception) {}
            }

            try {
                val serviceIntent = Intent(c, NotificationReceiver::class.java)
                c.startService(serviceIntent)
            } catch (e: Exception) {
                Log.w("NotificationReceiver", "startService failed, waiting for OS to bind service", e)
            }
        }
    }
}
