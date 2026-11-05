#!/usr/bin/env python3
"""
Antigravity PreToolUse Lifecycle Hook:
Routes command and tool execution confirmations to the user's Android phone
via KDE Connect with Apple Dynamic Island interface.
"""

import sys
import json
import subprocess
import os

def main():
    try:
        raw_input = sys.stdin.read()
        if not raw_input.strip():
            print(json.dumps({"decision": "allow"}))
            return 0
        payload = json.loads(raw_input)
    except Exception:
        # Fallback to allow if payload unreadable
        print(json.dumps({"decision": "allow"}))
        return 0

    tool_call = payload.get("toolCall", {})
    tool_name = tool_call.get("name", "unknown_tool")
    args = tool_call.get("args", {})

    # Extract command or action description
    command_line = args.get("CommandLine", "")
    tool_summary = args.get("toolSummary", tool_name)

    body_text = command_line if command_line else f"Tool: {tool_name}\nSummary: {tool_summary}"

    # Target prompt executable
    prompt_bin = "/home/mustofa/.gemini/antigravity/bin/kdeconnect-prompt"
    if not os.path.exists(prompt_bin):
        prompt_bin = os.path.expanduser("~/Workspace/Research/kdeconnect-kde-master/scripts/kdeconnect-prompt")

    try:
        cmd = [
            prompt_bin,
            "--title", "Antigravity Authorization",
            "--body", body_text,
            "--actions", "1:Allow Once", "2:Always Allow", "3:Deny",
            "--timeout", "45"
        ]
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        chosen_action = proc.stdout.strip()

        if proc.returncode == 0:
            if chosen_action in ["1", "2"] or "allow" in chosen_action.lower():
                print(json.dumps({"decision": "allow"}))
                return 0
            else:
                print(json.dumps({
                    "decision": "deny",
                    "reason": "Action denied by user via phone Dynamic Island prompt."
                }))
                return 0
        else:
            # If phone unreachable or timed out, fall back to desktop UI asking user
            print(json.dumps({"decision": "ask"}))
            return 0

    except Exception as e:
        sys.stderr.write(f"Hook error: {e}\n")
        print(json.dumps({"decision": "ask"}))
        return 0

if __name__ == "__main__":
    sys.exit(main())

