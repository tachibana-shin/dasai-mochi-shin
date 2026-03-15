This is the source code for esp32 c3 that I use for myself. Please see the GDPIO configuration in `config.h`. The qgif files need to be placed in `DasaiMochi/Mochi` on the memory card; there is no limit to the number of files.

# Scripts
- `remove_bg_gif.ps1` using ffmpeg allow remove background in gif shoot (from camera, youtube...). Filter default with `blue(150-255)`
- `convert_one_bit_gif.ps` using ffmpeg convert gif (removed background) to single color(black/white) for oled 1-bit
- `resize_gif.ps1` resize gif
