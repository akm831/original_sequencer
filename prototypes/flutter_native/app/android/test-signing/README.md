# Public test signing identity

This PKCS12 key is intentionally public and ONLY for local/CI debug APKs.
Store/key password: android. Alias: androiddebugkey.
Do not use it to sign a production release or establish a trusted distribution.
Stable identity lets subsequent prototype debug APKs update without uninstalling.
Previous ephemeral CI/local debug keys may differ; back up before migrating.
Certificate SHA-256: `154118501928c87f983dae83d87559452dfbaf5500a9f1be8e7aa7c4000357bf`.
