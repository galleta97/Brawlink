# BRAWLINK

Juego de acción 2D tipo stickman para Windows, hecho en **C++17** con **raylib**.
Proyecto del curso Desarrollo de Software (UTP).

## Cómo jugar

1. Instalar una sola vez CMake y el compilador:
   ```
   winget install Kitware.CMake
   winget install BrechtSanders.WinLibs.POSIX.UCRT
   ```
2. Doble clic en `compilar_y_ejecutar.bat` (la primera vez descarga raylib y tarda unos minutos).
3. Las siguientes veces basta con `jugar.bat`.

## Controles

| Acción | Teclado | Mando |
|---|---|---|
| Moverse | A / D o flechas | Stick / cruceta |
| Saltar | Espacio | A |
| Puñetazo / Patada | J / K | X / Y |
| Bloquear | L | B |
| Recoger arma | E | RB |
| Tiempo bala | Shift | LT |
| Pausa | Esc / P | Start |

## Estructura

```
Codigo/
├─ CMakeLists.txt   compila el juego y descarga raylib 5.5
├─ assets/fonts/    fuentes
└─ src/             una carpeta por clase (.h y .cpp juntos)
   ├─ core/         motor: Game, Scene, Input, Assets, Config
   ├─ game/         jugabilidad: Player, Enemy, World, Level, Stickman...
   ├─ scenes/       pantallas: login, menú, nivel, pausa, ranking
   └─ data/         perfiles y ranking
Documentacion/      presentación (PPT) y documento AV2
```

## Estado (20/11/2026)

- Sprint 1 y 2: completados (núcleo jugable, enemigos con IA, ragdoll propio, armas).
- Sprint 3 (en curso): tiempo bala, jefe Kage listo; faltan los niveles 2, 3 y 4.
- Sprint 4: perfiles, menús y ranking en archivos de texto; falta pasar a SQLite.
