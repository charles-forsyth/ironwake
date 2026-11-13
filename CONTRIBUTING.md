# Contributing to Ironwake

Contributions that improve hydrodynamic accuracy, sail & spar articulation, multi-port world tooling, or the interactive WebGL showcase are welcome.

## Development Workflow

1. Fork the repository and create a feature branch off `main`.
2. Keep `SPEC.md` and `spec/01..15_*.md` synchronized with any C++, JSON, or shader constant change.
3. Run both repository verification suites locally before opening a pull request:
   ```bash
   python3 tools/verify_repo.py
   python3 tools/spec_check.py --all
   ```
4. When modifying Unreal Engine 5.8 shaders or Python material builders (`Content/Python/`), avoid `MaterialExpressionClamp` so materials compile cleanly across Apple Silicon Metal SM6 (`SF_METAL_SM6`), Vulkan, and D3D12.
5. Open a pull request against `main` with clear before/after screenshots or telemetry metrics.

## Licensing

By submitting a pull request, you agree that your code contributions will be licensed under the repository's [MIT License](LICENSE), and that any contributed third-party art or scan assets carry compatible open licenses documented in [CREDITS.md](CREDITS.md).
