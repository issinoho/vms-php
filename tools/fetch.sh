#!/usr/bin/env bash
# fetch.sh - download the PHP release tarball named in upstream.conf into
# cache/, verify its SHA-256, and verify its GPG signature against
# keys/php-8.5-signing-keys.asc (the 8.5 release managers, D27).  Also the pinned CA bundle and Argon2 sources (SHA-256).
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
. "$top/upstream.conf"

cache=$top/cache
mkdir -p "$cache"
tarball=$cache/$(basename "$UPSTREAM_URL")

[ -f "$tarball" ] || { echo "fetch: downloading $UPSTREAM_URL"
                       curl -fsSL -o "$tarball.tmp" "$UPSTREAM_URL"; mv "$tarball.tmp" "$tarball"; }
[ -f "$tarball.asc" ] || curl -fsSL -o "$tarball.asc" "$UPSTREAM_URL.asc"

echo "$UPSTREAM_SHA256  $tarball" | sha256sum -c --quiet - ||
    { echo "fetch: SHA-256 mismatch for $tarball" >&2; exit 1; }

# A throwaway keyring holding only the pinned keys shipped in the repo.
keyring=$cache/php-pinned-keyring.gpg
rm -f "$keyring"
gpg --no-default-keyring --keyring "$keyring" --import "$top/keys/php-8.5-signing-keys.asc" 2>/dev/null
status=$(gpg --no-default-keyring --keyring "$keyring" --status-fd 1 \
             --verify "$tarball.asc" "$tarball" 2>/dev/null || true)
# VALIDSIG ends with the primary key's fingerprint; any one of the pinned keys will do.
signer=
for key in $UPSTREAM_GPG_KEYS; do
    echo "$status" | grep -q "^\[GNUPG:\] VALIDSIG .* $key\$" && signer=$key
done
[ -n "$signer" ] ||
    { echo "fetch: no valid signature by a pinned key ($UPSTREAM_GPG_KEYS) on $tarball" >&2; exit 1; }
echo "fetch: signature OK ($signer)"
echo "fetch: $tarball"

# the CA bundle for openssl.cafile (PORTING_LOG #57)
cabundle=$cache/$(basename "$CA_BUNDLE_URL")
[ -f "$cabundle" ] || { echo "fetch: downloading $CA_BUNDLE_URL"
                        curl -fsSL -o "$cabundle.tmp" "$CA_BUNDLE_URL"; mv "$cabundle.tmp" "$cabundle"; }
echo "$CA_BUNDLE_SHA256  $cabundle" | sha256sum -c --quiet - ||
    { echo "fetch: SHA-256 mismatch for $cabundle" >&2; exit 1; }
echo "fetch: $cabundle"

# Argon2's reference sources (D24)
argon2=$cache/phc-winner-argon2-$ARGON2_VERSION.tar.gz
[ -f "$argon2" ] || { echo "fetch: downloading $ARGON2_URL"
                      curl -fsSL -o "$argon2.tmp" "$ARGON2_URL"; mv "$argon2.tmp" "$argon2"; }
echo "$ARGON2_SHA256  $argon2" | sha256sum -c --quiet - ||
    { echo "fetch: SHA-256 mismatch for $argon2" >&2; exit 1; }
echo "fetch: $argon2"
