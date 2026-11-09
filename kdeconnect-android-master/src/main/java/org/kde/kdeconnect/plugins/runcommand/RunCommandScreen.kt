package org.kde.kdeconnect.plugins.runcommand

import androidx.activity.OnBackPressedDispatcher
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.GridItemSpan
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.ripple
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.runtime.snapshots.SnapshotStateList
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.input.pointer.PointerEventPass
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalHapticFeedback
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import org.kde.kdeconnect.Device
import org.kde.kdeconnect.ui.compose.KdeTheme
import org.kde.kdeconnect.ui.compose.KdeTopAppBar
import org.kde.kdeconnect.ui.compose.components.DeviceDisconnectedOverlay
import org.kde.kdeconnect_tp.R
import kotlin.time.Duration.Companion.seconds

@Composable
fun RunCommandScreen(
    plugin: RunCommandPlugin,
    device: Device,
    commandList: SnapshotStateList<CommandEntry>,
    onBackPressedDispatcher: OnBackPressedDispatcher,
    onCopyUrlToClipboard: (CommandEntry) -> Unit,
    onUpdate: () -> Unit
) {
    val haptics = LocalHapticFeedback.current
    val context = LocalContext.current
    var showDialog by remember { mutableStateOf(false) }
    val scope = rememberCoroutineScope()
    val outputList = remember { plugin.output }

    DisposableEffect(plugin) {
        val callback = RunCommandPlugin.CommandsChangedCallback {
            scope.launch(Dispatchers.Main.immediate) {
                onUpdate()
            }
        }
        plugin.addCommandsUpdatedCallback(callback)

        onDispose {
            plugin.removeCommandsUpdatedCallback(callback)
        }
    }

    val isReachable by device.isReachableFlow.collectAsStateWithLifecycle()

    val showOutputCard = commandList.isNotEmpty() &&
        device.canSendPacketType(RunCommandPlugin.PACKET_TYPE_RUNCOMMAND_OUTPUT)

    KdeTheme(context) {
        Scaffold(
            modifier = Modifier.safeDrawingPadding(),
            topBar = {
                RunCommandAppBar(
                    subTitle = device.name,
                    onBackPressedDispatcher = onBackPressedDispatcher,
                    showClearOutput = showOutputCard,
                    onClearOutput = { plugin.clearOutput() },
                )
            },
            floatingActionButton = {
                if (plugin.canAddCommand() && isReachable) {
                    FloatingActionButton(onClick = {
                        plugin.sendSetupPacket()
                        showDialog = true
                    }) {
                        Icon(
                            painterResource(R.drawable.ic_action_image_edit_24dp),
                            stringResource(R.string.add_command)
                        )
                    }
                }
            },
        ) {
            if (showDialog) {
                AlertDialog(
                    title = {
                        Text(
                            stringResource(R.string.add_command),
                            style = MaterialTheme.typography.titleLarge
                        )
                    },
                    text = {
                        Text(stringResource(R.string.add_command_description))
                    },
                    onDismissRequest = {
                        showDialog = false
                        onUpdate()
                    },
                    confirmButton = {
                        TextButton(onClick = {
                            showDialog = false
                            onUpdate()
                        }) {
                            Text(stringResource(R.string.ok))
                        }
                    },
                    dismissButton = {},
                )
            }

            if (!commandList.isEmpty()) {
                LazyVerticalGrid(
                    columns = GridCells.Fixed(2),
                    modifier = Modifier
                        .padding(it)
                        .fillMaxSize()
                        .padding(horizontal = 16.dp, vertical = 8.dp),
                    horizontalArrangement = Arrangement.spacedBy(12.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    if (showOutputCard) {
                        item(span = { GridItemSpan(2) }) {
                            OutputCard(outputList, plugin)
                        }
                    }

                    // Top banner: Stream Deck Macropad mode
                    item(span = { GridItemSpan(2) }) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clip(RoundedCornerShape(12.dp))
                                .background(Color(0xFF201B35))
                                .padding(horizontal = 14.dp, vertical = 10.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Box(
                                    modifier = Modifier
                                        .clip(CircleShape)
                                        .background(if (isReachable) Color(0xFF22C55E) else Color(0xFFEF4444))
                                        .padding(4.dp)
                                )
                                Spacer(modifier = Modifier.width(8.dp))
                                Text(
                                    text = "Stream Deck Macropad",
                                    color = Color.White,
                                    fontWeight = FontWeight.Bold,
                                    fontSize = 13.sp
                                )
                            }
                            Text(
                                text = "${commandList.size} Tombol Aktif",
                                color = Color(0x99CBD5E1),
                                fontSize = 11.sp
                            )
                        }
                    }

                    items(commandList) { command ->
                        StreamDeckItemCard(
                            command = command,
                            onRunCommand = { plugin.runCommand(command.key) },
                            onCopyUrlToClipboard = onCopyUrlToClipboard
                        )
                    }
                }
            } else {
                Column(
                    modifier = Modifier
                        .padding(it)
                        .fillMaxSize(),
                    verticalArrangement = Arrangement.Center,
                    horizontalAlignment = Alignment.CenterHorizontally,
                ) {
                    var text = stringResource(R.string.addcommand_explanation)
                    if (!plugin.canAddCommand()) {
                        text += "\n" + stringResource(R.string.addcommand_explanation2)
                    }
                    Text(text)
                }
            }

            DeviceDisconnectedOverlay(device, Modifier.padding(it))
        }
    }
}

@Composable
private fun StreamDeckItemCard(
    command: CommandEntry,
    onRunCommand: () -> Unit,
    onCopyUrlToClipboard: (CommandEntry) -> Unit,
    modifier: Modifier = Modifier
) {
    val haptics = LocalHapticFeedback.current
    var menuExpanded by remember { mutableStateOf(false) }
    var pressOffset by remember { mutableStateOf(IntOffset.Zero) }
    var isPressedFlash by remember { mutableStateOf(false) }

    LaunchedEffect(isPressedFlash) {
        if (isPressedFlash) {
            delay(300)
            isPressedFlash = false
        }
    }

    val nameLower = command.name.lowercase()
    val cmdLower = command.command.lowercase()
    val (badgeText, badgeColor, cardBorder) = when {
        "git" in nameLower || "git" in cmdLower -> Triple("GIT", Color(0xFFF97316), Color(0x66F97316))
        "lock" in nameLower || "kunci" in nameLower -> Triple("LOCK", Color(0xFFF43F5E), Color(0x66F43F5E))
        "auth" in nameLower || "sidik" in nameLower -> Triple("AUTH", Color(0xFFFBBF24), Color(0x66FBBF24))
        "dev" in nameLower || "build" in nameLower || "daemon" in nameLower || "server" in nameLower -> Triple("DEV", Color(0xFF06B6D4), Color(0x6606B6D4))
        "audio" in nameLower || "mute" in nameLower || "sound" in nameLower -> Triple("AUDIO", Color(0xFFA855F7), Color(0x66A855F7))
        "webcam" in nameLower || "cam" in nameLower || "camera" in nameLower -> Triple("CAM", Color(0xFFEC4899), Color(0x66EC4899))
        "prox" in nameLower || "walk" in nameLower -> Triple("PROX", Color(0xFF10B981), Color(0x6610B981))
        "term" in nameLower || "terminal" in nameLower -> Triple("TERM", Color(0xFF14B8A6), Color(0x6614B8A6))
        else -> Triple("MACRO", Color(0xFF3B82F6), Color(0x663B82F6))
    }

    Box(modifier = modifier) {
        Card(
            shape = RoundedCornerShape(18.dp),
            border = BorderStroke(
                1.5.dp,
                if (isPressedFlash) badgeColor else cardBorder
            ),
            colors = CardDefaults.cardColors(
                containerColor = if (isPressedFlash) Color(0xFF2C2448) else Color(0xFF181528)
            ),
            modifier = Modifier
                .fillMaxWidth()
                .height(130.dp)
                .pointerInput(command.key) {
                    awaitEachGesture {
                        val down = awaitFirstDown(
                            requireUnconsumed = false,
                            pass = PointerEventPass.Initial,
                        )
                        pressOffset = IntOffset(
                            down.position.x.toInt(),
                            down.position.y.toInt(),
                        )
                    }
                }
                .combinedClickable(
                    indication = ripple(),
                    interactionSource = remember { MutableInteractionSource() },
                    onClick = {
                        haptics.performHapticFeedback(HapticFeedbackType.LongPress)
                        isPressedFlash = true
                        onRunCommand()
                    },
                    onLongClick = {
                        haptics.performHapticFeedback(HapticFeedbackType.LongPress)
                        menuExpanded = true
                    }
                )
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(14.dp),
                verticalArrangement = Arrangement.SpaceBetween
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Box(
                        modifier = Modifier
                            .clip(RoundedCornerShape(6.dp))
                            .background(badgeColor.copy(alpha = 0.2f))
                            .padding(horizontal = 7.dp, vertical = 3.dp)
                    ) {
                        Text(
                            text = badgeText,
                            color = badgeColor,
                            fontSize = 10.sp,
                            fontWeight = FontWeight.ExtraBold,
                            letterSpacing = 1.sp
                        )
                    }

                    Box(
                        modifier = Modifier
                            .clip(CircleShape)
                            .background(badgeColor.copy(alpha = if (isPressedFlash) 1f else 0.4f))
                            .padding(4.dp)
                    )
                }

                Column {
                    Text(
                        text = command.name,
                        maxLines = 2,
                        overflow = TextOverflow.Ellipsis,
                        color = Color.White,
                        fontSize = 14.sp,
                        fontWeight = FontWeight.Bold,
                        lineHeight = 18.sp
                    )

                    Spacer(modifier = Modifier.height(3.dp))

                    Text(
                        text = command.command,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        color = Color(0x88CBD5E1),
                        fontSize = 10.sp
                    )
                }
            }
        }

        Box(modifier = Modifier.offset { pressOffset }) {
            DropdownMenu(
                expanded = menuExpanded,
                onDismissRequest = { menuExpanded = false },
            ) {
                DropdownMenuItem(
                    text = { Text(stringResource(R.string.copy_url_to_clipboard)) },
                    onClick = {
                        menuExpanded = false
                        onCopyUrlToClipboard(command)
                    },
                )
            }
        }
    }
}

@Composable
private fun OutputCard(
    outputList: SnapshotStateList<RunCommandOutput>,
    plugin: RunCommandPlugin
) {
    val state = rememberLazyListState()
    val coroutineScope = rememberCoroutineScope()
    val commandRunning by plugin.commandRunning
    var showStopButton by remember { mutableStateOf(false) }

    LaunchedEffect(commandRunning) {
        if (commandRunning) {
            delay(1.seconds)
            showStopButton = true
        } else {
            showStopButton = false
        }
    }

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .height(300.dp)
            .padding(horizontal = 20.dp, vertical = 10.dp)
    ) {
        if (outputList.isNotEmpty()) {
            Box {
                LazyColumn(
                    modifier = Modifier
                        .padding(horizontal = 15.dp, vertical = 5.dp)
                        .fillMaxWidth(),
                    state = state
                ) {
                    items(outputList) { text ->
                        Text(
                            modifier = Modifier
                                .fillMaxWidth(),
                            text = text.string,
                            style = MaterialTheme.typography.bodyLarge,
                            fontWeight = if (text.isCommand) FontWeight.ExtraBold else FontWeight.Normal,
                            color = when (text.commandStatus) {
                                RunCommandStatus.COMMAND_SUCCESSFUL -> Color(0xFF2E7D32)
                                RunCommandStatus.COMMAND_FAILED -> MaterialTheme.colorScheme.error
                                else -> Color.Unspecified
                            }
                        )
                    }
                }
                if (showStopButton) {
                    Column(
                        modifier = Modifier.padding(5.dp).fillMaxSize(),
                        horizontalAlignment = Alignment.End,
                        verticalArrangement = Arrangement.Bottom
                    ) {
                        IconButton(onClick = { plugin.sendStop() }) {
                            CircularProgressIndicator()
                            Icon(
                                painterResource(R.drawable.ic_stop),
                                stringResource(R.string.runcommand_stop)
                            )
                        }
                    }
                }
            }
        } else {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(20.dp),
                verticalArrangement = Arrangement.Center,
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                Text(
                    text = stringResource(R.string.runcommand_output_no_output),
                    textAlign = TextAlign.Center,
                    fontWeight = FontWeight.ExtraBold,
                    style = MaterialTheme.typography.bodyLarge
                )
                Text(
                    text = stringResource(R.string.runcommand_output_no_output_desc),
                    textAlign = TextAlign.Center,
                    style = MaterialTheme.typography.bodyLarge
                )
            }
        }
    }

    LaunchedEffect(outputList.size) {
        if (outputList.isNotEmpty()) {
            coroutineScope.launch {
                state.animateScrollToItem(outputList.size - 1)
            }
        }
    }
}

@Composable
fun RunCommandAppBar(
    subTitle: String,
    onBackPressedDispatcher: OnBackPressedDispatcher,
    showClearOutput: Boolean,
    onClearOutput: () -> Unit,
) {
    KdeTopAppBar(
        title = stringResource(R.string.pref_plugin_runcommand),
        subTitle = subTitle,
        navIconOnClick = { onBackPressedDispatcher.onBackPressed() },
        navIconDescription = stringResource(androidx.appcompat.R.string.abc_action_bar_up_description),
        actions = {
            if (showClearOutput) {
                var menuExpanded by remember { mutableStateOf(false) }
                IconButton(onClick = { menuExpanded = true }) {
                    Icon(Icons.Default.MoreVert, stringResource(R.string.extra_options))
                }
                DropdownMenu(
                    expanded = menuExpanded,
                    onDismissRequest = { menuExpanded = false },
                ) {
                    DropdownMenuItem(
                        text = { Text(stringResource(R.string.runcommand_clear_output)) },
                        onClick = {
                            menuExpanded = false
                            onClearOutput()
                        },
                    )
                }
            }
        },
    )
}
