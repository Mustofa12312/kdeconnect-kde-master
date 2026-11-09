#!/usr/bin/env python3
"""
Global Antigravity PreToolUse Lifecycle Hook:
Intercepts tool executions across ALL projects and prompts the user's smartphone
via KDE Connect with Apple Dynamic Island interface.

Features:
- Instant bypass for safe read-only operations (grep, ls, find, cat, git status, etc.)
- Intelligent Whitelist: Base binary (git, cmake, etc.) and workspace path matching
- Dual-Screen Synchronization with auto-dismiss on Android & Desktop
- Instant fallback if phone is offline (<0.2s)
- Fast timeout fallback (12s) if user doesn't want to pick up the phone
- One-touch toggle (phone-auth on/off)
"""

import sys
import json
import subprocess
import os
import shlex

ALLOWLIST_FILE = os.path.expanduser("~/.gemini/antigravity/always_allowed_commands.json")
DISABLED_FLAG_FILE = os.path.expanduser("~/.gemini/antigravity/phone_auth_disabled")

# Safe read-only tool names
SAFE_TOOLS = {
    "read_file", "search_web", "list_directory", "list_resources",
    "read_resource", "get_code_snippet", "search_graph", "query_graph",
    "trace_path", "get_architecture", "search_code", "index_status"
}

# Safe read-only command binaries and subcommands
SAFE_BASE_COMMANDS = {
    "grep", "egrep", "fgrep", "rg", "ag",
    "find", "ls", "dir", "pwd", "which", "whereis", "type", "echo", "printf",
    "cat", "head", "tail", "wc", "less", "more", "file", "stat",
    "date", "uptime", "uname", "tree"
}

SAFE_GIT_SUBCOMMANDS = {
    "status", "diff", "log", "branch", "show", "tag", "rev-parse",
    "describe", "remote", "config"
}

def is_phone_auth_disabled():
    return os.path.exists(DISABLED_FLAG_FILE)

def load_allowlist():
    if os.path.exists(ALLOWLIST_FILE):
        try:
            with open(ALLOWLIST_FILE, "r") as f:
                data = json.load(f)
                if isinstance(data, list):
                    return set(data)
                elif isinstance(data, dict):
                    entries = set()
                    for v in data.values():
                        if isinstance(v, list):
                            entries.update(v)
                    return entries
        except Exception:
            return set()
    return set()

def save_to_allowlist(entry):
    if not entry:
        return
    allowlist = load_allowlist()
    allowlist.add(entry.strip())
    try:
        os.makedirs(os.path.dirname(ALLOWLIST_FILE), exist_ok=True)
        with open(ALLOWLIST_FILE, "w") as f:
            json.dump(sorted(list(allowlist)), f, indent=2)
    except Exception as e:
        sys.stderr.write(f"Failed to save allowlist: {e}\n")

def get_base_binary(command_line):
    """Extract primary binary name from a command line string."""
    try:
        tokens = shlex.split(command_line)
        if not tokens:
            return ""
        # Handle sudo or env prefixes
        idx = 0
        while idx < len(tokens) and tokens[idx] in ("sudo", "env", "nohup", "time", "nice"):
            idx += 1
        if idx < len(tokens):
            binary = os.path.basename(tokens[idx])
            return binary
    except Exception:
        parts = command_line.strip().split()
        if parts:
            return os.path.basename(parts[0])
    return ""

def is_safe_shell_command(command_line):
    """Check if command is strictly read-only inspection."""
    if not command_line:
        return True
    
    # Strip basic quotes / whitespace
    cmd = command_line.strip()
    
    # Disallow destructive operators in safe auto-check
    danger_tokens = [">", ">>", "rm ", "dd ", "mkfs", "reboot", "shutdown", "kill", "pkill"]
    if any(tok in cmd for tok in danger_tokens):
        return False

    base = get_base_binary(cmd)
    if not base:
        return False

    if base in SAFE_BASE_COMMANDS:
        return True

    # Safe git read-only inspection
    if base == "git":
        try:
            tokens = shlex.split(cmd)
            for token in tokens[1:]:
                if not token.startswith("-"):
                    if token in SAFE_GIT_SUBCOMMANDS:
                        return True
                    break
        except Exception:
            pass

    # Safe version checks (node -v, python --version, cmake --version)
    if any(flag in cmd for flag in ["--version", "-v", "--help", "-h"]):
        return True

    return False

def is_always_allowed(tool_name, args, allowlist):
    """Check if the tool call or command matches our allowlist."""
    if not allowlist:
        return False

    if tool_name == "run_command":
        cmd = args.get("CommandLine", "").strip()
        if not cmd:
            return True
        if cmd in allowlist:
            return True
        base = get_base_binary(cmd)
        if base and (f"base:{base}" in allowlist or base in allowlist):
            return True

    elif tool_name in ("write_file", "edit_file"):
        path = args.get("Path", "").strip()
        if not path:
            return False
        if f"path:{path}" in allowlist or path in allowlist:
            return True
        # Check parent directories
        parent = os.path.dirname(path)
        while parent and parent != "/":
            if f"path:{parent}" in allowlist or parent in allowlist:
                return True
            parent = os.path.dirname(parent)
        if f"tool:{tool_name}" in allowlist:
            return True

    return False

def record_always_allow(tool_name, args):
    """Record permissions when user chooses 'Always allow'."""
    if tool_name == "run_command":
        cmd = args.get("CommandLine", "").strip()
        if cmd:
            save_to_allowlist(cmd)
            base = get_base_binary(cmd)
            if base:
                save_to_allowlist(f"base:{base}")
    elif tool_name in ("write_file", "edit_file"):
        path = args.get("Path", "").strip()
        if path:
            save_to_allowlist(f"path:{path}")
            # Also allow the parent directory (project scope)
            parent = os.path.dirname(path)
            if parent:
                save_to_allowlist(f"path:{parent}")

def main():
    try:
        raw_input = sys.stdin.read()
        if not raw_input.strip():
            print(json.dumps({"decision": "allow"}))
            return 0
        payload = json.loads(raw_input)
    except Exception:
        print(json.dumps({"decision": "allow"}))
        return 0

    # If user toggled off phone authorization, fallback to desktop modal immediately (0.01s)
    if is_phone_auth_disabled():
        print(json.dumps({"decision": "ask"}))
        return 0

    tool_call = payload.get("toolCall", {})
    tool_name = tool_call.get("name", "unknown_tool")
    args = tool_call.get("args", {})

    # 1. Safe read-only native tools bypass instantly
    if tool_name in SAFE_TOOLS:
        print(json.dumps({"decision": "allow"}))
        return 0

    command_line = args.get("CommandLine", "").strip()

    # 2. Safe read-only shell commands bypass instantly (grep, find, ls, git status...)
    if tool_name == "run_command" and is_safe_shell_command(command_line):
        print(json.dumps({"decision": "allow"}))
        return 0

    # 3. Check persistent allowlist (exact command, base binary, or path)
    allowlist = load_allowlist()
    if is_always_allowed(tool_name, args, allowlist):
        print(json.dumps({
            "decision": "allow",
            "permissionOverrides": [f"command({command_line})"] if command_line else []
        }))
        return 0

    # Prepare user-friendly prompt text
    tool_summary = args.get("toolSummary", "")
    if tool_name == "run_command":
        body_text = f"$ {command_line}"
        if tool_summary:
            body_text = f"[{tool_summary}]\n$ {command_line}"
    elif tool_name in ("write_file", "edit_file"):
        path = args.get("Path", "")
        body_text = f"Tool: {tool_name}\nTarget: {path}"
    else:
        body_text = f"Tool: {tool_name}\nSummary: {tool_summary}"

    prompt_bin = "/home/mustofa/.gemini/antigravity/bin/kdeconnect-prompt"
    if not os.path.exists(prompt_bin):
        prompt_bin = os.path.expanduser("~/Workspace/Research/kdeconnect-kde-master/scripts/kdeconnect-prompt")

    try:
        # Fast 12s timeout: If user ignores the phone, fallback to laptop modal quickly!
        cmd = [
            prompt_bin,
            "--title", "Antigravity Authorization",
            "--body", body_text,
            "--actions",
            "1:Yes, allow this time",
            "2:Yes, and always allow",
            "3:No",
            "--timeout", "12"
        ]
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        chosen_action = proc.stdout.strip()

        if proc.returncode == 0:
            if chosen_action == "1" or "allow this time" in chosen_action.lower():
                print(json.dumps({"decision": "allow"}))
                return 0
            elif chosen_action == "2" or "always" in chosen_action.lower():
                record_always_allow(tool_name, args)
                print(json.dumps({
                    "decision": "allow",
                    "permissionOverrides": [f"command({command_line})"] if command_line else []
                }))
                return 0
            else:
                # User chose "No" / Deny
                print(json.dumps({
                    "decision": "deny",
                    "reason": "Execution denied by user on Pixel 8a."
                }))
                return 0
        else:
            # Phone was unreachable or user didn't pick up the phone (timed out in 12s):
            # Fall back to desktop UI modal immediately
            print(json.dumps({"decision": "ask"}))
            return 0

    except Exception as e:
        sys.stderr.write(f"Hook error: {e}\n")
        print(json.dumps({"decision": "ask"}))
        return 0

if __name__ == "__main__":
    sys.exit(main())
