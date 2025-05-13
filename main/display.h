//
// display.h
// Project TrollNFC
// 
// Created by Lakr233 on 2025/05/12.
//

#ifndef DISPLAY_H

#include <stdio.h>
#include <string.h>

void initialize_display(void);
void display_text(const char *text);
void display_text_with_format(const char *text, ...);

#endif
