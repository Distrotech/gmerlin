/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

/*
 *  System independent keycode definitions
 */

/** \defgroup keycodes System independent keycode definitions
 *  \ingroup plugin_ov
 *
 *  @{
 */

#define BG_KEY_SHIFT_MASK    (1<<0) //!< Shift
#define BG_KEY_CONTROL_MASK  (1<<1) //!< Control
#define BG_KEY_ALT_MASK      (1<<2) //!< Alt
#define BG_KEY_SUPER_MASK    (1<<3) //!< Windows key is called "Super" under X11
#define BG_KEY_BUTTON1_MASK  (1<<4) //!< Mouse button 1
#define BG_KEY_BUTTON2_MASK  (1<<5) //!< Mouse button 2
#define BG_KEY_BUTTON3_MASK  (1<<6) //!< Mouse button 3
#define BG_KEY_BUTTON4_MASK  (1<<7) //!< Mouse button 4
#define BG_KEY_BUTTON5_MASK  (1<<8) //!< Mouse button 5

#define BG_KEY_NONE      -1 //!< Undefined

#define BG_KEY_0          0 //!< 0
#define BG_KEY_1          1 //!< 1
#define BG_KEY_2          2 //!< 2
#define BG_KEY_3          3 //!< 3
#define BG_KEY_4          4 //!< 4
#define BG_KEY_5          5 //!< 5
#define BG_KEY_6          6 //!< 6
#define BG_KEY_7          7 //!< 7
#define BG_KEY_8          8 //!< 8
#define BG_KEY_9          9 //!< 9

#define BG_KEY_SPACE      10 //!< Space
#define BG_KEY_RETURN     11 //!< Return (Enter)
#define BG_KEY_LEFT       12 //!< Left
#define BG_KEY_RIGHT      13 //!< Right
#define BG_KEY_UP         14 //!< Up
#define BG_KEY_DOWN       15 //!< Down
#define BG_KEY_PAGE_UP    16 //!< Page Up
#define BG_KEY_PAGE_DOWN  17 //!< Page Down
#define BG_KEY_HOME       18 //!< Page Down
#define BG_KEY_PLUS       19 //!< Plus
#define BG_KEY_MINUS      20 //!< Minus
#define BG_KEY_TAB        21 //!< Tab
#define BG_KEY_ESCAPE     22 //!< Esc
#define BG_KEY_MENU       23 //!< Menu key

#define BG_KEY_QUESTION   24 //!< ?
#define BG_KEY_EXCLAM     25 //!< !
#define BG_KEY_QUOTEDBL   26 //!< "
#define BG_KEY_DOLLAR     27 //!< $
#define BG_KEY_PERCENT    28 //!< %
#define BG_KEY_APMERSAND  29 //!< &
#define BG_KEY_SLASH      30 //!< /
#define BG_KEY_LEFTPAREN  31 //!< (
#define BG_KEY_RIGHTPAREN 32 //!< )
#define BG_KEY_EQUAL      33 //!< =
#define BG_KEY_BACKSLASH  34 //!< :-)

#define BG_KEY_A 101 //!< A
#define BG_KEY_B 102 //!< B
#define BG_KEY_C 103 //!< C
#define BG_KEY_D 104 //!< D
#define BG_KEY_E 105 //!< E
#define BG_KEY_F 106 //!< F
#define BG_KEY_G 107 //!< G
#define BG_KEY_H 108 //!< H
#define BG_KEY_I 109 //!< I
#define BG_KEY_J 110 //!< J
#define BG_KEY_K 111 //!< K
#define BG_KEY_L 112 //!< L
#define BG_KEY_M 113 //!< M
#define BG_KEY_N 114 //!< N
#define BG_KEY_O 115 //!< O
#define BG_KEY_P 116 //!< P
#define BG_KEY_Q 117 //!< Q
#define BG_KEY_R 118 //!< R
#define BG_KEY_S 119 //!< S
#define BG_KEY_T 120 //!< T
#define BG_KEY_U 121 //!< U
#define BG_KEY_V 122 //!< V
#define BG_KEY_W 123 //!< W
#define BG_KEY_X 124 //!< X
#define BG_KEY_Y 125 //!< Y
#define BG_KEY_Z 126 //!< Z

#define BG_KEY_a 201 //!< a
#define BG_KEY_b 202 //!< b
#define BG_KEY_c 203 //!< c
#define BG_KEY_d 204 //!< d
#define BG_KEY_e 205 //!< e
#define BG_KEY_f 206 //!< f
#define BG_KEY_g 207 //!< g
#define BG_KEY_h 208 //!< h
#define BG_KEY_i 209 //!< i
#define BG_KEY_j 210 //!< j
#define BG_KEY_k 211 //!< k
#define BG_KEY_l 212 //!< l
#define BG_KEY_m 213 //!< m
#define BG_KEY_n 214 //!< n
#define BG_KEY_o 215 //!< o
#define BG_KEY_p 216 //!< p
#define BG_KEY_q 217 //!< q
#define BG_KEY_r 218 //!< r
#define BG_KEY_s 219 //!< s
#define BG_KEY_t 220 //!< t
#define BG_KEY_u 221 //!< u
#define BG_KEY_v 222 //!< v
#define BG_KEY_w 223 //!< w
#define BG_KEY_x 224 //!< x
#define BG_KEY_y 225 //!< y
#define BG_KEY_z 226 //!< z


#define BG_KEY_F1  301 //!< F1
#define BG_KEY_F2  302 //!< F2
#define BG_KEY_F3  303 //!< F3
#define BG_KEY_F4  304 //!< F4
#define BG_KEY_F5  305 //!< F5
#define BG_KEY_F6  306 //!< F6
#define BG_KEY_F7  307 //!< F7
#define BG_KEY_F8  308 //!< F8
#define BG_KEY_F9  309 //!< F9
#define BG_KEY_F10 310 //!< F10
#define BG_KEY_F11 311 //!< F11
#define BG_KEY_F12 312 //!< F12

/**  @}
 */
