#!/usr/bin/env python3
"""
Global Antigravity PreToolUse Lifecycle Hook:
Intercepts tool executions across ALL projects and prompts the user's smartphone
via KDE Connect with Apple Dynamic Island interface.

Includes:
- Instant fallback if phone is offline (<0.2s)
- Fast timeout fallback (12s) if user doesn't want to pick up the phone
- One-touch toggle (phone-auth on/off)
- Whitelist cache for "Always Allow"
"""

import sys
import json
import subprocess
import os

ALLOWLIST_FILE = os.path.expanduser("~/.gemini/antigravity/always_allowed_commands.json")
DISABLED_FLAG_FILE = os.path.expanduser("~/.gemini/antigravity/phone_auth_disabled")

def is_phone_auth_disabled():
    return os.path.exists(DISABLED_FLAG_FILE)

def load_allowlist():
    if os.path.exists(ALLOWLIST_FILE):
        try:
            with open(ALLOWLIST_FILE, "r") as f:
                return set(json.load(f))
        except Exception:
            return set()
    return set()

def save_to_allowlist(command):
    allowlist = load_allowlist()
    allowlist.add(command)
    try:
        os.makedirs(os.path.dirname(ALLOWLIST_FILE), exist_ok=True)
        with open(ALLOWLIST_FILE, "w") as f:
            json.dump(list(allowlist), f, indent=2)
    except Exception as e:
        sys.stderr.write(f"Failed to save allowlist: {e}\n")

def is_always_allowed(command):
    if not command:
        return False
    allowlist = load_allowlist()
    return command.strip() in allowlist

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

    # Safe read-only tools don't require authorization
    safe_tools = {
        "read_file", "search_web", "list_directory", "list_resources",
        "read_resource", "get_code_snippet", "search_graph", "query_graph",
        "trace_path", "get_architecture", "search_code", "index_status"
    }
    if tool_name in safe_tools:
        print(json.dumps({"decision": "allow"}))
        return 0

    command_line = args.get("CommandLine", "").strip()
    tool_summary = args.get("toolSummary", tool_name)

    # If user previously chose "Always allow" for this command
    if command_line and is_always_allowed(command_line):
        print(json.dumps({
            "decision": "allow",
            "permissionOverrides": [f"command({command_line})"]
        }))
        return 0

    body_text = command_line if command_line else f"Tool: {tool_name}\nSummary: {tool_summary}"

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
                if command_line:
                    save_to_allowlist(command_line)
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
