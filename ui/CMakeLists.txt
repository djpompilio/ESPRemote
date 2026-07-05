SET(SOURCES ui_comp_button1.c
    ui_comp.c
    ui_Home.c
    ui_Menu.c
    ui_Stats.c
    ui_theme_manager.c
    ui_themes.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_wifi_h_png.c
    fonts/ui_font_SFRound_20.c
    fonts/ui_font_SFRound_16.c
    fonts/ui_font_SFRound_BOLD_16.c
    fonts/ui_font_SFRound_BOLD_20.c)

add_library(ui ${SOURCES})
