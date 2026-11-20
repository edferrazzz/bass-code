# Bass Code

Gerador de linhas de baixo + synth da Audio Makers. Simples por fora, complexo por dentro.
Escolha o estilo (PSY, HOUSE, AFRO HOUSE, BRAZILIAN BASS, ELETRÔNICA), aperte GERAR, edite no piano roll
e arraste o MIDI ou o WAV (com lowpass/sweep, reverb, delay e sidechain) pra DAW.

- JUCE 8.0.12, C++20, CMake. Standalone + VST3 (Windows), + AU no macOS.
- `Source/Core/Music.cpp`: estilos, grooves, progressões e gerador.
- `Source/Dsp/Synth.cpp`: synth mono (30 timbres, 4 macros SUB/DRIVE/MOVIMENTO/PUNCH).
- `Source/Dsp/Fx.cpp`: LOWPASS (+SWEEP), SIDECHAIN, DELAY, REVERB, limiter.
- Testes: `bass_tests` (gerador + áudio de todos os timbres). `BASS_SNAPSHOT=x.png` gera a tela, `BASS_DEMO_DIR=dir` gera WAVs.
- CI (`.github/workflows/build.yml`): build MSVC + instalador Inno, commitado em `Release/Windows/`.
