#!/usr/bin/env bash
#
# Git Phone Authorization Hook via KDE Connect
# Requires user confirmation on smartphone before completing sensitive git operations.
#

ACTION_TYPE="${1:-pre-push}"
PROMPT_BIN="/home/mustofa/.gemini/antigravity/bin/kdeconnect-prompt"
if [ ! -x "$PROMPT_BIN" ]; then
    PROMPT_BIN="$(dirname "$0")/kdeconnect-prompt"
fi

if [ "$ACTION_TYPE" = "pre-push" ]; then
    REMOTE="${2:-origin}"
    URL="${3:-}"
    BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
    RECENT_COMMIT="$(git log -1 --pretty=format:'%h: %s' 2>/dev/null || echo 'no commit')"

    BODY="Action: Git Push\nBranch: ${BRANCH}\nRemote: ${REMOTE}\nCommit: ${RECENT_COMMIT}"
    TITLE="Git Push Authorization"
elif [ "$ACTION_TYPE" = "pre-commit" ]; then
    BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
    STAGED_COUNT="$(git diff --cached --numstat | wc -l)"

    BODY="Action: Git Commit\nBranch: ${BRANCH}\nFiles staged: ${STAGED_COUNT}"
    TITLE="Git Commit Authorization"
else
    BODY="Action: ${ACTION_TYPE}\nRepo: $(basename "$PWD")"
    TITLE="Git Security Authorization"
fi

if [ ! -x "$PROMPT_BIN" ]; then
    echo "[Git-Auth] kdeconnect-prompt not found, skipping."
    exit 0
fi

echo "[Git-Auth] Requesting authorization on Pixel 8a..."
CHOICE="$("$PROMPT_BIN" --title "$TITLE" --body "$BODY" --actions "1:Allow" "2:Deny" --timeout 45 2>/dev/null)"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ "$CHOICE" = "1" ] || [[ "$CHOICE" =~ [Aa]llow ]]; then
        echo "[Git-Auth] Authorized via Pixel 8a. Proceeding."
        exit 0
    else
        echo "[Git-Auth] Aborted: Denied by user on Pixel 8a."
        exit 1
    fi
else
    echo "[Git-Auth] Phone prompt timed out or unreachable."
    read -r -p "Authorize on terminal? [y/N]: " REPLY
    case "$REPLY" in
        [yY][eE][sS]|[yY])
            exit 0
            ;;
        *)
            echo "[Git-Auth] Push aborted."
            exit 1
            ;;
    esac
fi

