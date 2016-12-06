/*=========================================================================*/
/**
 * \file	CursorControl.c
 *
 * \brief	Functions for handling application cursor change requests.
 *
 *          The widget making the request is added to a list and a destroy
 *          callback added.  When the calling widget is destroyed it is
 *          removed from the internal list. XuWINDOW_OBSCURED_CURSOR is a
 *          special case in that the obscured cursor will override every
 *          other cursor request while set except for the busy cursor.
 *
 *     Version 8 (c) Copyright 2011 Environment Canada
 *
 *   This file is part of the Forecast Production Assistant (FPA).
 *   The FPA is free software: you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   any later version.
 *
 *   The FPA is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *   See the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
 */
/*=========================================================================*/

#include "XuP.h"

/* Define this for debug printing output */
#undef DEBUG_PRINT


/* Finger Pointing Cursor
*/
#define fp_width 30
#define fp_height 28
#define fp_x_hot 0
#define fp_y_hot 2
static unsigned char fp_bits[] = {
   0x03, 0x00, 0xf8, 0x3f, 0x03, 0x00, 0xf0, 0x3f, 0xfc, 0xff, 0xe7, 0x3f,
   0xfc, 0xff, 0xcf, 0x3f, 0x03, 0xc0, 0x9f, 0x3f, 0x03, 0xc0, 0x9f, 0x3f,
   0x3f, 0xff, 0x9f, 0x3f, 0x3f, 0xff, 0x9f, 0x3f, 0xff, 0xc0, 0x9f, 0x3f,
   0xff, 0xc0, 0x9f, 0x3f, 0x3f, 0xff, 0x1f, 0x3f, 0x3f, 0xff, 0x4f, 0x3e,
   0xff, 0xc0, 0xe7, 0x3c, 0xff, 0xc0, 0xf3, 0x39, 0x3f, 0xff, 0xf9, 0x33,
   0x3f, 0xff, 0xfc, 0x27, 0xff, 0x70, 0xfe, 0x0f, 0xff, 0x20, 0xff, 0x0f,
   0xff, 0x87, 0xff, 0x27, 0xff, 0xcf, 0xff, 0x33, 0xff, 0xe7, 0xf9, 0x39,
   0xff, 0xcf, 0xf9, 0x3c, 0xff, 0x9f, 0x7f, 0x3e, 0xff, 0x3f, 0x3f, 0x3f,
   0xff, 0x7f, 0x9e, 0x3f, 0xff, 0xff, 0xcc, 0x3f, 0xff, 0xff, 0xe1, 0x3f,
   0xff, 0xff, 0xf3, 0x3f};
static unsigned char fp_mask[] = {
   0xfc, 0xff, 0x07, 0x00, 0xfc, 0xff, 0x0f, 0x00, 0xff, 0xff, 0x1f, 0x00,
   0xff, 0xff, 0x3f, 0x00, 0xfc, 0xff, 0x7f, 0x00, 0xfc, 0xff, 0x7f, 0x00,
   0xc0, 0xff, 0x7f, 0x00, 0xc0, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00,
   0x00, 0xff, 0x7f, 0x00, 0xc0, 0xff, 0xff, 0x00, 0xc0, 0xff, 0xff, 0x01,
   0x00, 0xff, 0xff, 0x03, 0x00, 0xff, 0xff, 0x07, 0xc0, 0xff, 0xff, 0x0f,
   0xc0, 0xff, 0xff, 0x1f, 0x00, 0xff, 0xff, 0x3f, 0x00, 0xff, 0xff, 0x3f,
   0x00, 0xf8, 0xff, 0x1f, 0x00, 0xf0, 0xff, 0x0f, 0x00, 0xf8, 0xff, 0x07,
   0x00, 0xf0, 0xff, 0x03, 0x00, 0xe0, 0xff, 0x01, 0x00, 0xc0, 0xff, 0x00,
   0x00, 0x80, 0x7f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x1e, 0x00,
   0x00, 0x00, 0x0c, 0x00};


/* Hourglass Cursor
*/
#define hg_width 32
#define hg_height 32
#define hg_x_hot 15
#define hg_y_hot 15
static unsigned char hg_bits[] = {
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03, 0x40, 0x00, 0x00, 0x01,
   0xe0, 0xff, 0xff, 0x03, 0x60, 0x00, 0x00, 0x03, 0x60, 0x00, 0x00, 0x03,
   0x60, 0x00, 0x00, 0x03, 0x60, 0xc0, 0x01, 0x03, 0xc0, 0xa8, 0x8e, 0x01,
   0x80, 0x55, 0xdd, 0x00, 0x00, 0xab, 0x6f, 0x00, 0x00, 0x56, 0x37, 0x00,
   0x00, 0xec, 0x1b, 0x00, 0x00, 0xd8, 0x0d, 0x00, 0x00, 0xb0, 0x06, 0x00,
   0x00, 0xa0, 0x02, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0xb0, 0x06, 0x00,
   0x00, 0x98, 0x0c, 0x00, 0x00, 0x8c, 0x18, 0x00, 0x00, 0x06, 0x30, 0x00,
   0x00, 0x83, 0x60, 0x00, 0x80, 0xc1, 0xc3, 0x00, 0xc0, 0xb0, 0x86, 0x01,
   0x60, 0x5c, 0x1d, 0x03, 0x60, 0x2a, 0x39, 0x03, 0x60, 0xdd, 0x7f, 0x03,
   0x60, 0x00, 0x00, 0x03, 0xe0, 0xff, 0xff, 0x03, 0x40, 0x00, 0x00, 0x01,
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03};
static unsigned char hg_mask[] = {
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0x01,
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03,
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0x01,
   0x80, 0xff, 0xff, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xfe, 0x3f, 0x00,
   0x00, 0xfc, 0x1f, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00,
   0x00, 0xe0, 0x03, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
   0x00, 0xf8, 0x0f, 0x00, 0x00, 0xfc, 0x1f, 0x00, 0x00, 0xfe, 0x3f, 0x00,
   0x00, 0xff, 0x7f, 0x00, 0x80, 0xff, 0xff, 0x00, 0xc0, 0xff, 0xff, 0x01,
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03,
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0x01,
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03};


/* Knife Cursor
*/
#define knife_width 30
#define knife_height 26
#define knife_x_hot 0
#define knife_y_hot 25
static unsigned char knife_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x03,
   0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x60, 0x0e, 0x00, 0x00, 0x30, 0x1c,
   0x00, 0x00, 0x18, 0x3c, 0x00, 0x00, 0x0c, 0x1a, 0x00, 0x00, 0x06, 0x0d,
   0x00, 0x00, 0x83, 0x06, 0x00, 0x80, 0x41, 0x03, 0x00, 0xc0, 0xa0, 0x01,
   0x00, 0x60, 0xd0, 0x00, 0x00, 0x30, 0x68, 0x00, 0x00, 0x18, 0x34, 0x00,
   0x00, 0x2c, 0x1a, 0x00, 0x00, 0x56, 0x0d, 0x00, 0x00, 0xab, 0x06, 0x00,
   0x80, 0x55, 0x03, 0x00, 0xc0, 0xa8, 0x01, 0x00, 0x60, 0xd0, 0x00, 0x00,
   0x30, 0x60, 0x00, 0x00, 0x18, 0x30, 0x00, 0x00, 0x0c, 0x18, 0x00, 0x00,
   0xfe, 0x0f, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00};
static unsigned char knife_mask[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x03,
   0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0xe0, 0x0f, 0x00, 0x00, 0xf0, 0x1f,
   0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xfc, 0x1f, 0x00, 0x00, 0xfe, 0x0f,
   0x00, 0x00, 0xff, 0x07, 0x00, 0x80, 0xff, 0x03, 0x00, 0xc0, 0xff, 0x01,
   0x00, 0xe0, 0xff, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xfc, 0x1f, 0x00, 0x00, 0xfe, 0x0f, 0x00, 0x00, 0xff, 0x07, 0x00,
   0x80, 0xff, 0x03, 0x00, 0xc0, 0xff, 0x01, 0x00, 0xe0, 0xff, 0x00, 0x00,
   0xf0, 0x7f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xfc, 0x1f, 0x00, 0x00,
   0xfe, 0x0f, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00};


/* Magnification Cursor
*/
#define mag_width 31
#define mag_height 31
#define mag_x_hot 11
#define mag_y_hot 11
static unsigned char mag_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00,
   0xc0, 0xff, 0x01, 0x00, 0xe0, 0xc1, 0x03, 0x00, 0x70, 0x00, 0x07, 0x00,
   0x38, 0x00, 0x0e, 0x00, 0x18, 0x08, 0x0c, 0x00, 0x1c, 0x08, 0x1c, 0x00,
   0x0c, 0x08, 0x18, 0x00, 0x0c, 0x08, 0x18, 0x00, 0x8c, 0xff, 0x18, 0x00,
   0x0c, 0x08, 0x18, 0x00, 0x0c, 0x08, 0x18, 0x00, 0x1c, 0x08, 0x1c, 0x00,
   0x18, 0x08, 0x0c, 0x00, 0x38, 0x00, 0x1e, 0x00, 0x70, 0x00, 0x3f, 0x00,
   0xe0, 0xc1, 0x73, 0x00, 0xc0, 0xff, 0xe3, 0x00, 0x00, 0x7f, 0xc7, 0x01,
   0x00, 0x00, 0x8e, 0x03, 0x00, 0x00, 0x1c, 0x07, 0x00, 0x00, 0x38, 0x0e,
   0x00, 0x00, 0x70, 0x1c, 0x00, 0x00, 0xe0, 0x38, 0x00, 0x00, 0xc0, 0x71,
   0x00, 0x00, 0x80, 0x3b, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x0e,
   0x00, 0x00, 0x00, 0x04};
static unsigned char mag_mask[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00,
   0xc0, 0xff, 0x01, 0x00, 0xe0, 0xc1, 0x03, 0x00, 0x70, 0x00, 0x07, 0x00,
   0x38, 0x00, 0x0e, 0x00, 0x18, 0x08, 0x0c, 0x00, 0x1c, 0x08, 0x1c, 0x00,
   0x0c, 0x08, 0x18, 0x00, 0x0c, 0x08, 0x18, 0x00, 0x8c, 0xff, 0x18, 0x00,
   0x0c, 0x08, 0x18, 0x00, 0x0c, 0x08, 0x18, 0x00, 0x1c, 0x08, 0x1c, 0x00,
   0x18, 0x08, 0x0c, 0x00, 0x38, 0x00, 0x1e, 0x00, 0x70, 0x00, 0x3f, 0x00,
   0xe0, 0xc1, 0x73, 0x00, 0xc0, 0xff, 0xe3, 0x00, 0x00, 0x7f, 0xc7, 0x01,
   0x00, 0x00, 0x8e, 0x03, 0x00, 0x00, 0x1c, 0x07, 0x00, 0x00, 0x38, 0x0e,
   0x00, 0x00, 0x70, 0x1c, 0x00, 0x00, 0xe0, 0x38, 0x00, 0x00, 0xc0, 0x71,
   0x00, 0x00, 0x80, 0x3b, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x0e,
   0x00, 0x00, 0x00, 0x04};


/* Obscured Cursor
*/
#define obs_width 31
#define obs_height 31
#define obs_x_hot 15
#define obs_y_hot 15
static unsigned char obs_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0xfe, 0x7f, 0x00,
   0x00, 0x0f, 0xf0, 0x00, 0xc0, 0x03, 0xc0, 0x03, 0xe0, 0x00, 0x00, 0x07,
   0x70, 0xff, 0x07, 0x0e, 0x30, 0xff, 0x07, 0x0c, 0x18, 0xff, 0x07, 0x18,
   0x1c, 0xff, 0x07, 0x38, 0x0c, 0xff, 0xff, 0x31, 0x0c, 0xff, 0xff, 0x31,
   0x06, 0xff, 0x87, 0x61, 0x06, 0xff, 0x87, 0x61, 0x06, 0xff, 0x87, 0x61,
   0x06, 0xff, 0x87, 0x61, 0x06, 0xff, 0x87, 0x61, 0x06, 0xff, 0x87, 0x61,
   0x06, 0x0c, 0x80, 0x61, 0x06, 0x0c, 0x80, 0x61, 0x0c, 0x0c, 0x80, 0x31,
   0x0c, 0x0c, 0x80, 0x31, 0x1c, 0xfc, 0xff, 0x39, 0x18, 0xfc, 0xff, 0x19,
   0x30, 0x00, 0x00, 0x0c, 0x70, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x00, 0x07,
   0xc0, 0x03, 0xc0, 0x03, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0xfe, 0x7f, 0x00,
   0x00, 0xf0, 0x0f, 0x00};
static unsigned char obs_mask[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0xfe, 0x7f, 0x00,
   0x00, 0x0f, 0xf0, 0x00, 0xc0, 0x03, 0xc0, 0x03, 0xe0, 0x00, 0x00, 0x07,
   0x70, 0xff, 0x07, 0x0e, 0x30, 0xff, 0x07, 0x0c, 0x18, 0xff, 0x07, 0x18,
   0x1c, 0xff, 0x07, 0x38, 0x0c, 0xff, 0xff, 0x31, 0x0c, 0xff, 0xff, 0x31,
   0x06, 0xff, 0x87, 0x61, 0x06, 0xff, 0x87, 0x61, 0x06, 0xff, 0x87, 0x61,
   0x06, 0xff, 0x87, 0x61, 0x06, 0xff, 0x87, 0x61, 0x06, 0xff, 0x87, 0x61,
   0x06, 0x0c, 0x80, 0x61, 0x06, 0x0c, 0x80, 0x61, 0x0c, 0x0c, 0x80, 0x31,
   0x0c, 0x0c, 0x80, 0x31, 0x1c, 0xfc, 0xff, 0x39, 0x18, 0xfc, 0xff, 0x19,
   0x30, 0x00, 0x00, 0x0c, 0x70, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x00, 0x07,
   0xc0, 0x03, 0xc0, 0x03, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0xfe, 0x7f, 0x00,
   0x00, 0xf0, 0x0f, 0x00};


/* Pan Cursor
*/
#define pan_width 35
#define pan_height 35
#define pan_x_hot 17
#define pan_y_hot 17
static unsigned char pan_bits[] = {
   0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00,
   0x07, 0x00, 0x00, 0x00, 0x80, 0x0f, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00,
   0x00, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0x00,
   0xe0, 0x3f, 0x00, 0x00, 0x00, 0x30, 0x67, 0x00, 0x00, 0x00, 0x00, 0x07,
   0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
   0x00, 0x01, 0x07, 0x06, 0x00, 0xc0, 0x01, 0x07, 0x1c, 0x00, 0xf0, 0x00,
   0x07, 0x7c, 0x00, 0xf8, 0x00, 0x07, 0xf8, 0x00, 0xfe, 0xff, 0xff, 0xff,
   0x03, 0xff, 0xff, 0xff, 0xff, 0x07, 0xfe, 0xff, 0xff, 0xff, 0x03, 0xf8,
   0x00, 0x07, 0xf8, 0x00, 0xe0, 0x00, 0x07, 0x38, 0x00, 0xc0, 0x01, 0x07,
   0x1c, 0x00, 0x00, 0x01, 0x07, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
   0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x30,
   0x67, 0x00, 0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0x00, 0xe0, 0x3f, 0x00,
   0x00, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x00,
   0x80, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x07,
   0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00};
static unsigned char pan_mask[] = {
   0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00,
   0x07, 0x00, 0x00, 0x00, 0x80, 0x0f, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00,
   0x00, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0x00,
   0xe0, 0x3f, 0x00, 0x00, 0x00, 0x30, 0x67, 0x00, 0x00, 0x00, 0x00, 0x07,
   0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
   0x00, 0x01, 0x07, 0x06, 0x00, 0xc0, 0x01, 0x07, 0x1c, 0x00, 0xf0, 0x00,
   0x07, 0x7c, 0x00, 0xf8, 0x00, 0x07, 0xf8, 0x00, 0xfe, 0xff, 0xff, 0xff,
   0x03, 0xff, 0xff, 0xff, 0xff, 0x07, 0xfe, 0xff, 0xff, 0xff, 0x03, 0xf8,
   0x00, 0x07, 0xf8, 0x00, 0xe0, 0x00, 0x07, 0x38, 0x00, 0xc0, 0x01, 0x07,
   0x1c, 0x00, 0x00, 0x01, 0x07, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
   0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x30,
   0x67, 0x00, 0x00, 0x00, 0xe0, 0x3f, 0x00, 0x00, 0x00, 0xe0, 0x3f, 0x00,
   0x00, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x00,
   0x80, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x07,
   0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00};



/* Pencil Cursor
*/
#define pen_width 26
#define pen_height 26
#define pen_x_hot 0
#define pen_y_hot 22
static unsigned char pen_bits[] = {
   0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x4c, 0x00,
   0x00, 0x00, 0x96, 0x00, 0x00, 0x00, 0x29, 0x01, 0x00, 0x80, 0x50, 0x03,
   0x00, 0x40, 0xa8, 0x01, 0x00, 0x20, 0xe4, 0x00, 0x00, 0x10, 0x72, 0x00,
   0x00, 0x08, 0x39, 0x00, 0x00, 0x84, 0x1c, 0x00, 0x00, 0x42, 0x0e, 0x00,
   0x00, 0x21, 0x07, 0x00, 0x80, 0x90, 0x03, 0x00, 0x40, 0xc8, 0x01, 0x00,
   0x20, 0xe4, 0x00, 0x00, 0x10, 0x72, 0x00, 0x00, 0x18, 0x39, 0x00, 0x00,
   0xb8, 0x1c, 0x00, 0x00, 0x6c, 0x0e, 0x00, 0x00, 0xd4, 0x07, 0x00, 0x00,
   0xac, 0x03, 0x00, 0x00, 0xd6, 0x01, 0x00, 0x00, 0x6a, 0x00, 0x00, 0x00,
   0x1f, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00};
static unsigned char pen_mask[] = {
   0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x7c, 0x00,
   0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00, 0x80, 0xff, 0x03,
   0x00, 0xc0, 0xff, 0x01, 0x00, 0xe0, 0xff, 0x00, 0x00, 0xf0, 0x7f, 0x00,
   0x00, 0xf8, 0x3f, 0x00, 0x00, 0xfc, 0x1f, 0x00, 0x00, 0xfe, 0x0f, 0x00,
   0x00, 0xff, 0x07, 0x00, 0x80, 0xff, 0x03, 0x00, 0xc0, 0xff, 0x01, 0x00,
   0xe0, 0xff, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00,
   0xf8, 0x1f, 0x00, 0x00, 0xfc, 0x0f, 0x00, 0x00, 0xfc, 0x07, 0x00, 0x00,
   0xfc, 0x03, 0x00, 0x00, 0xfe, 0x01, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00,
   0x1f, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00};


/* Sample Cursor
*/
#define samp_width 23
#define samp_height 23
#define samp_x_hot 11
#define samp_y_hot 11
static unsigned char samp_bits[] = {
   0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00, 0x7f, 0x00, 0xc0, 0xff, 0x01,
   0xe0, 0xc9, 0x03, 0x70, 0x08, 0x07, 0x38, 0x08, 0x0e, 0x18, 0x08, 0x0c,
   0x1c, 0x00, 0x1c, 0x0c, 0x00, 0x18, 0x0c, 0x08, 0x18, 0xff, 0x9c, 0x7f,
   0x0c, 0x08, 0x18, 0x0c, 0x00, 0x18, 0x1c, 0x00, 0x1c, 0x18, 0x08, 0x0c,
   0x38, 0x08, 0x0e, 0x70, 0x08, 0x07, 0xe0, 0xc9, 0x03, 0xc0, 0xff, 0x01,
   0x00, 0x7f, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00};
static unsigned char samp_mask[] = {
   0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00, 0x7f, 0x00, 0xc0, 0xff, 0x01,
   0xe0, 0xc9, 0x03, 0x70, 0x08, 0x07, 0x38, 0x08, 0x0e, 0x18, 0x08, 0x0c,
   0x1c, 0x00, 0x1c, 0x0c, 0x00, 0x18, 0x0c, 0x08, 0x18, 0xff, 0x9c, 0x7f,
   0x0c, 0x08, 0x18, 0x0c, 0x00, 0x18, 0x1c, 0x00, 0x1c, 0x18, 0x08, 0x0c,
   0x38, 0x08, 0x0e, 0x70, 0x08, 0x07, 0xe0, 0xc9, 0x03, 0xc0, 0xff, 0x01,
   0x00, 0x7f, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00};


/* Stopsign Cursor
*/
#define stop_width 22
#define stop_height 22
#define stop_x_hot 8
#define stop_y_hot 8
static unsigned char stop_bits[] = {
   0x00, 0x3f, 0x00, 0xe0, 0xff, 0x01, 0xf0, 0xff, 0x03, 0xf8, 0xc0, 0x07,
   0x7c, 0x00, 0x0f, 0xfe, 0x00, 0x1e, 0xfe, 0x01, 0x1c, 0xee, 0x03, 0x1c,
   0xc7, 0x07, 0x38, 0x87, 0x0f, 0x38, 0x07, 0x1f, 0x38, 0x07, 0x3e, 0x38,
   0x07, 0x7c, 0x38, 0x07, 0xf8, 0x38, 0x0e, 0xf0, 0x1d, 0x0e, 0xe0, 0x1f,
   0x1e, 0xc0, 0x1f, 0x3c, 0x80, 0x0f, 0xf8, 0xc0, 0x07, 0xf0, 0xff, 0x03,
   0xe0, 0xff, 0x01, 0x00, 0x3f, 0x00};
static unsigned char stop_mask[] = {
   0x00, 0x3f, 0x00, 0xe0, 0xff, 0x01, 0xf0, 0xff, 0x03, 0xf8, 0xc0, 0x07,
   0x7c, 0x00, 0x0f, 0xfe, 0x00, 0x1e, 0xfe, 0x01, 0x1c, 0xee, 0x03, 0x1c,
   0xc7, 0x07, 0x38, 0x87, 0x0f, 0x38, 0x07, 0x1f, 0x38, 0x07, 0x3e, 0x38,
   0x07, 0x7c, 0x38, 0x07, 0xf8, 0x38, 0x0e, 0xf0, 0x1d, 0x0e, 0xe0, 0x1f,
   0x1e, 0xc0, 0x1f, 0x3c, 0x80, 0x0f, 0xf8, 0xc0, 0x07, 0xf0, 0xff, 0x03,
   0xe0, 0xff, 0x01, 0x00, 0x3f, 0x00};


/* Value Modification Cursor
*/
#define vm_width 31
#define vm_height 36
#define vm_x_hot 16
#define vm_y_hot 18
static unsigned char vm_bits[] = {
   0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
   0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00, 0xf8, 0x0f, 0x00,
   0x00, 0xdc, 0x1d, 0x00, 0x00, 0xdc, 0x1d, 0x00, 0x00, 0xce, 0x39, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00, 0xff, 0xff, 0xff, 0x7f,
   0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f,
   0xff, 0xff, 0xff, 0x7f, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xce, 0x39, 0x00, 0x00, 0xdc, 0x1d, 0x00, 0x00, 0xdc, 0x1d, 0x00,
   0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00, 0xe0, 0x03, 0x00,
   0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00};
static unsigned char vm_mask[] = {
   0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
   0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00, 0xf8, 0x0f, 0x00,
   0x00, 0xdc, 0x1d, 0x00, 0x00, 0xdc, 0x1d, 0x00, 0x00, 0xce, 0x39, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00, 0xff, 0xff, 0xff, 0x7f,
   0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f,
   0xff, 0xff, 0xff, 0x7f, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
   0x00, 0xce, 0x39, 0x00, 0x00, 0xdc, 0x1d, 0x00, 0x00, 0xdc, 0x1d, 0x00,
   0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00, 0xe0, 0x03, 0x00,
   0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00};

/* Structure to hold information on the cursors available to the application.
*  The cursors are obtained from the cursors defined at the beginning of this
*  file. The order must agree with that in the Xu.h header file. XuBUSY_CURSOR
*  must remain the first entry in the array and XuVALUEMOD_CURSOR followed by
*  XuDEFAULT_CURSOR and XuMENU_CURSOR the last item.
*/
#define BLACK "black"
#define WHITE "white"
#define YELLOW "yellow"
#define RED    "red"

static XuCursorDataStruct cursor_data[] =
{
	/* XuBUSY_CURSOR */
	{ hg_width, hg_height, hg_bits, hg_mask, hg_x_hot, hg_y_hot, BLACK,  WHITE },
	/* XuWINDOW_OBSCURED_CURSOR */
	{obs_width, obs_height, obs_bits, obs_mask, obs_x_hot, obs_y_hot, YELLOW,  WHITE },
	/* XuSTOP_CURSOR */
	{ stop_width, stop_height, stop_bits, stop_mask, stop_x_hot, stop_y_hot, RED,  WHITE },
	/* XuFINGER_CURSOR */
	{ fp_width, fp_height, fp_bits, fp_mask, fp_x_hot, fp_y_hot, BLACK,  WHITE },
	/* XuKNIFE_CURSOR */
	{ knife_width, knife_height, knife_bits, knife_mask, knife_x_hot, knife_y_hot, BLACK,  WHITE },
	/* XuMAGNIFY_CURSOR */
	{ mag_width, mag_height, mag_bits, mag_mask, mag_x_hot, mag_y_hot, YELLOW,  WHITE },
	/* XuPAN_CURSOR */
	{ pan_width, pan_height, pan_bits, pan_mask, pan_x_hot, pan_y_hot, YELLOW,  WHITE },
	/* XuPENCIL_CURSOR */
	{ pen_width, pen_height, pen_bits, pen_mask, pen_x_hot, pen_y_hot, BLACK,  WHITE },
	/* XuSAMPLE_CURSOR */
	{ samp_width, samp_height, samp_bits, samp_mask, samp_x_hot, samp_y_hot, YELLOW,  WHITE },
	/* XuVALUEMOD_CURSOR */
	{ vm_width, vm_height, vm_bits, vm_mask, vm_x_hot, vm_y_hot, YELLOW,  WHITE },
	/* XuDEFAULT_CURSOR */
	{ 0, 0, NULL, NULL, 0, 0, BLACK,  WHITE },
	/* XuMENU_CURSOR */
	{ 0, 0, NULL, NULL, 0, 0, BLACK,  WHITE }
};

#undef BLACK
#undef WHITE
#undef YELLOW
#undef RED


#define HIST_LEN 6

/* Define structure to hold temporary setting information
*/
typedef struct {
	Widget        w;
	XuCURSOR_TYPE cursor_id;
	Boolean       state;
	int           count;
} TOSET;

/* Define structure to hold cursor display history.
*/
typedef struct {
	Widget  w;				/* Widget for which the cursor is valid */
	Cursor  clist[HIST_LEN];/* Cursor history list, 0 is most recent */
	Boolean obscured;		/* Has obscured cursor been set? */
	Boolean stop_sign;		/* Has stop sign cursor been set? */
	DPYPTR  dp;				/* display structure pointer - see XuP.h */
} CURHIST;


/* Local static variables
*/
static int     history_len = 0;
static CURHIST *history = NULL;
static int     busy_cursor_count = 0;



/* The cursor returned is dependent on the display of the given widget.
*  Keep the crated cursors separate depending on the display in which
*  a given widget exists.
*/
static Cursor GetCursor(int hist_posn, int type )
{
	XColor   xc[2];
	Pixmap   pixmap, mask;
	Colormap cmap;
	DPYPTR   dp;
	static Boolean no_menu_cursor_query = True;

	if(!history[hist_posn].w) return None;

	dp = history[hist_posn].dp;

	if(dp->cursor[type] == None && NotNull(cursor_data[type].bits))
	{
		Window win = XRootWindowOfScreen(XtScreen(history[hist_posn].w));
		xc[0].pixel = XuLoadColor(history[hist_posn].w,cursor_data[type].fg);
		xc[1].pixel = XuLoadColor(history[hist_posn].w,cursor_data[type].bg);
		XtVaGetValues(CW(history[hist_posn].w), XmNcolormap, &cmap, NULL);
		XQueryColors(dp->display, cmap, xc, 2);
		pixmap = XCreateBitmapFromData(dp->display, win, (char *)cursor_data[type].bits,
			cursor_data[type].width, cursor_data[type].height);
		mask = XCreateBitmapFromData(dp->display, win, (char *)cursor_data[type].mask,
			cursor_data[type].width, cursor_data[type].height);			
		dp->cursor[type] = XCreatePixmapCursor(dp->display, pixmap, mask,
			&xc[0], &xc[1], cursor_data[type].x_hot, cursor_data[type].y_hot);
		XFreePixmap(dp->display,pixmap);
		XFreePixmap(dp->display,mask);
		XFlush(dp->display);
		XSync(dp->display, False);
	}

	/* This is just paranoia to prevent infinite recursion should something go wrong
	*/
	if(no_menu_cursor_query)
	{
		/* The menu cursor is a special case and is run automatically.
		*/
		if(dp->cursor[XuMENU_CURSOR] == None &&  NotNull(cursor_data[XuMENU_CURSOR].bits))
		{
			no_menu_cursor_query = False;
			XmSetMenuCursor(dp->display, GetCursor(hist_posn, XuMENU_CURSOR));
			no_menu_cursor_query = True;
		}
	}
	return dp->cursor[type];
}


/*  SelectInput() - Restrict input to the widgets while the busy cursor
*                     is showing by changing the event mask of the widgets.
*   This should hopefully reduce problems on a slow system with people
*   pushing buttons while the cursor is busy.
*/
static void SelectInput(Widget _w, Boolean _set)
{
	static long busy_mask = {
		LeaveWindowMask          | Button1MotionMask  | Button2MotionMask      | 
		Button3MotionMask        | Button4MotionMask  | Button5MotionMask      |
		ButtonMotionMask         | ExposureMask       | VisibilityChangeMask   |
		StructureNotifyMask      | ResizeRedirectMask | SubstructureNotifyMask |
		SubstructureRedirectMask | FocusChangeMask    | PropertyChangeMask     |
		ColormapChangeMask       | OwnerGrabButtonMask
	};

	int        i, nw;
	WidgetList wl;

	if(XtIsComposite(_w))
	{
		XtVaGetValues(_w, XtNnumChildren, &nw, XtNchildren, &wl, NULL);
		for(i = 0; i < nw; i++)
			SelectInput(wl[i], _set);
	}
	if(XtIsWidget(_w))
	{
		XSelectInput(XtDisplay(_w), XtWindowOfObject(_w), (_set)? XtBuildEventMask(_w):busy_mask);
	}
}



/*=========================================================================*/
/**
 * \brief	Set the default cursor for the entire application.
 *
 * \param[in] new A structure that defines the cursor.
 *
 * \attention
 *      The cursor definition stored in the location pointed in by new must
 *      remain static in the calling program while the cursor is in effect.
 *
 *  <b>Cursor Structure</b>
 *
 * \verbatim
 unsigned int  width, height;   cursor width and height
 unsigned char *bits;           cursor definition
 unsigned char *mask;           masking bits
 unsigned int  x_hot, y_hot;    the cursor hot spot location
 char          *fg, *bg;        foreground and background colour names
 \endverbatim
 *
 * The bits and mask variables are arrays which define the cursor and
 * are of a form that must conform to the requirements of the Xlib
 * XCreateBitmapFromData function.
 */
/*=========================================================================*/
void XuSetDefaultCursor(XuCursorDataStruct *new )
{
	int     i, j, k;
	Cursor  newcur, oldcur;

	if(new) (void) memcpy((void*)&cursor_data[XuDEFAULT_CURSOR], (void*)new, sizeof(XuCursorDataStruct));
	else    (void) memset((void*)&cursor_data[XuDEFAULT_CURSOR], 0, sizeof(XuCursorDataStruct));
		
	/* Clear the existing default cursor lists
	*/
	for(k = 0; k < Fxu.ndd; k++)
	{
		oldcur = Fxu.dd[k]->cursor[XuDEFAULT_CURSOR];
		Fxu.dd[k]->cursor[XuDEFAULT_CURSOR] = None;

		for( i = 0; i < history_len; i++ )
		{
			if(IsNull(history[i].w) || history[i].dp != Fxu.dd[k]) continue;
			newcur = GetCursor(i,XuDEFAULT_CURSOR);
			for(j = 0; j < HIST_LEN; j++)
			{
				if(history[i].clist[j] == oldcur) history[i].clist[j] = newcur;
			}
			if(history[i].clist[0] == newcur)
			{
				XUndefineCursor(XtDisplayOfObject(history[i].w), XtWindowOfObject(history[i].w));
				XDefineCursor(XtDisplayOfObject(history[i].w), XtWindowOfObject(history[i].w), newcur);
				XuUpdateDisplay(history[i].w);
			}
		}
		if(oldcur!= None) XFreeCursor(Fxu.dd[k]->display, oldcur);
	}
}


/*=========================================================================*/
/**
 * \brief	Set the default menu cursor for the entire application.
 *
 * \param[in] new A structure that defines the menu cursor.
 *
 * \attention
 *      The cursor definition stored in the location pointed in by new must
 *      remain static in the calling program while the cursor is in effect.
 *
 *  <b>Cursor Structure</b>
 *
 * \verbatim
 unsigned int  width, height;   cursor width and height
 unsigned char *bits;           cursor definition
 unsigned char *mask;           masking bits
 unsigned int  x_hot, y_hot;    the cursor hot spot location
 char          *fg, *bg;        foreground and background colour names
 \endverbatim
 *
 * The bits and mask variables are arrays which define the cursor and
 * are of a form that must conform to the requirements of the Xlib
 * XCreateBitmapFromData function.
 */
/*=========================================================================*/
void XuSetDefaultMenuCursor(XuCursorDataStruct *new )
{
	int     i, j;
	Cursor  oldcur;

	if(new) (void) memcpy((void*)&cursor_data[XuMENU_CURSOR], (void*)new, sizeof(XuCursorDataStruct));
	else    (void) memset((void*)&cursor_data[XuMENU_CURSOR], 0, sizeof(XuCursorDataStruct));

	for( i = 0; i < Fxu.ndd; i++ )
	{
		oldcur = Fxu.dd[i]->cursor[XuMENU_CURSOR];
		Fxu.dd[i]->cursor[XuMENU_CURSOR] = None;
		for( j = 0; j < history_len; j++ )
		{
			if(IsNull(history[j].w) || history[j].dp != Fxu.dd[i]) continue;
			XmSetMenuCursor(Fxu.dd[i]->display, GetCursor(j, XuMENU_CURSOR));
			break;
		}
		if(oldcur != None) XFreeCursor(Fxu.dd[i]->display, oldcur);
	}
}


/*=========================================================================*/
/**
 * \brief Show or hide a busy cursor in every active window in the
 *        application.
 *
 * \param[in] set True or False
 *
 * \attention
 *		If the busy cursor is set x times then it must be turned off x
 *		times before the cursor is reset to the non-busy state.
 */
/*=========================================================================*/
void XuSetBusyCursor(const Boolean set )
{
	int     i;
	Cursor  cursor;
	Display *dpy = (Display*)NULL;

	if(set && busy_cursor_count < 0) busy_cursor_count = 0;
	(set)? busy_cursor_count++:busy_cursor_count--;
	if(busy_cursor_count < 0 || busy_cursor_count > 1) return;

	for( i = 0; i < history_len; i++ )
	{
		if(!history[i].w) continue;
		if(!XtIsWidget(history[i].w)) continue;
		if(!XtIsRealized(history[i].w)) continue;

		if(busy_cursor_count > 0)
			cursor = GetCursor(i, XuBUSY_CURSOR);
		else if(history[i].stop_sign)
			cursor = GetCursor(i, XuSTOP_CURSOR);
		else if(history[i].obscured)
			cursor = GetCursor(i, XuWINDOW_OBSCURED_CURSOR);
		else
			cursor = history[i].clist[0];

		dpy = XtDisplayOfObject(history[i].w);
		XUndefineCursor(dpy, XtWindowOfObject(history[i].w));
		XDefineCursor(dpy, XtWindowOfObject(history[i].w), cursor);
		XuUpdateDisplay(history[i].w);
	}
}


/*=========================================================================*/
/**
 * \brief Turns off the busy cursor no matter how many times it has been
 *        set on.
 */
/*=========================================================================*/
void XuClearBusyCursor(void)
{
	busy_cursor_count = 0;
	XuSetBusyCursor(False);
}


/*=========================================================================*/
/**
 * \brief Set the cursor to the requested type for the diaog which
 *        contains the given widget
 *
 * \param[in] w A widget belonging to the dialog
 * \param[in] cursor_id The requested cursor
 * \param[in] state If the cursor is on or off (True or False)
 *
 * <b>Cursor Types:</b>
 *
 * \arg \c XuBUSY_CURSOR An hourglass
 * \arg \c XuWINDOW_OBSCURED_CURSOR A red no go circle
 * \arg \c XuSTOP_CURSOR Stopsign
 * \arg \c XuFINGER_CURSOR Pointing finger
 * \arg \c XuKNIFE_CURSOR Cutting knife
 * \arg \c XuMAGNIFY_CURSOR Magnifying glass
 * \arg \c XuPAN_CURSOR Cross hair
 * \arg \c XuPENCIL_CURSOR Drawing pencil
 * \arg \c XuSAMPLE_CURSOR Sample cross hairs
 * \arg \c XuVALUEMOD_CURSOR Modification indication
 * \arg \c XuDEFAULT_CURSOR As set by XuSetDefaultCursor
 * \arg \c XuMENU_CURSOR As set by XuSetDefaultMenuCursor
 * 
 */
/*=========================================================================*/
void XuSetDialogCursor(Widget w, XuCURSOR_TYPE cursor_id ,const Boolean state )
{
	int i;
	Widget top, wt;

	top = XuGetShell(w);
	XuSetCursor(top, cursor_id, state);
	for( i = 0; i < history_len; i++ )
	{
		if(!history[i].w) continue;
		if(!XtIsWidget(history[i].w)) continue;
		if(!XtIsRealized(history[i].w)) continue;
		wt = XuGetShell(history[i].w);
		if(wt != top || history[i].w == wt) continue;
		XuSetCursor(history[i].w, cursor_id, state);
	}
}


/*=========================================================================*/
/**
 * \brief Set the given dialog cursor directly to the given cursor.
 *
 * \param[in] w A widget in the dialog
 * \param[in] cursor The cursor use in the dialog
 *
 * \attention
 *   If the cursor is not valid for the display the dialog is on the
 *   function will result in a fatal error.
 */
/*=========================================================================*/
void XuSetDialogCursorDirect(Widget w, Cursor cursor)
{
	int    i;
	Widget top, wt;

	top = XuGetShell(w);
	XUndefineCursor(XtDisplayOfObject(top), XtWindowOfObject(top));
	XDefineCursor(XtDisplayOfObject(top), XtWindowOfObject(top), cursor);
	
	for( i = 0; i < history_len; i++ )
	{
		if(!history[i].w) continue;
		wt = XuGetShell(history[i].w);
		if(wt != top || history[i].w == wt) continue;
		XUndefineCursor(XtDisplayOfObject(history[i].w), XtWindowOfObject(history[i].w));
		XDefineCursor(XtDisplayOfObject(history[i].w), XtWindowOfObject(history[i].w), cursor);
	}
	XuUpdateDisplay(w);
}



/*ARGSUSED*/
static void DelayedCursorSetCB(XtPointer client_data , XtIntervalId id )
{
	TOSET *ptr = (TOSET *)client_data;

	/* The loop count prevents this time out callback from repeating
	 * forever if the widget is never realized for some reason. The
	 * cutoff is 1 minute.
	 */
	if(ptr->count > 60)
	{
		XtFree((void*) ptr);
	}
	else if(XtIsRealized(ptr->w))
	{
		XuSetCursor(ptr->w, ptr->cursor_id, ptr->state);
		XtFree((void*) ptr);
	}
	else
	{
		ptr->count++;
		(void)XtAppAddTimeOut(XtWidgetToApplicationContext(ptr->w), 1000,
				(XtTimerCallbackProc)DelayedCursorSetCB, client_data);
	}
}




/*ARGSUSED*/
static void PurgeCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	(void) memset((void *)(history+PTR2INT(client_data)), 0, sizeof(CURHIST));
}


/*=========================================================================*/
/**
 * \brief Set the cursor for the given widget to the requested type.
 *
 * \param[in] w The widget to set the cursor for
 * \param[in] cursor_id The cursor selecton
 * \param[in] state True or False
 *
 * <b>Cursor Types:</b>
 *
 * \arg \c XuBUSY_CURSOR An hourglass
 * \arg \c XuWINDOW_OBSCURED_CURSOR A red no go circle
 * \arg \c XuSTOP_CURSOR Stopsign
 * \arg \c XuFINGER_CURSOR Pointing finger
 * \arg \c XuKNIFE_CURSOR Cutting knife
 * \arg \c XuMAGNIFY_CURSOR Magnifying glass
 * \arg \c XuPAN_CURSOR Cross hair
 * \arg \c XuPENCIL_CURSOR Drawing pencil
 * \arg \c XuSAMPLE_CURSOR Sample cross hairs
 * \arg \c XuVALUEMOD_CURSOR Modification indication
 * \arg \c XuDEFAULT_CURSOR As set by XuSetDefaultCursor
 * \arg \c XuMENU_CURSOR As set by XuSetDefaultMenuCursor
 *
 * \note
 *   The state parameter controls the action to be taken with the cursor.
 *   If True, the given cursor_id is to be set as the active cursor. If
 *   False, the given cursor_id is to be removed as the active cursor and
 *   the cursor returned to the cursor previously set by a call to this
 *   function. Note that the specified cursor_id will be removed even if it
 *   is not actually showing as the cursor in the application (ie. the busy
 *   cursor or obscured cursor are active).
 */
/*=========================================================================*/

/*   If the widget is not yet realized we put the request into a delay for
*   one second and try to set the cursor again for the specified widget.
*   This is done as we must have a window id to set the cursor. Note that
*   if the busy cursor is showing this is taken as an override for the set
*   cursor and we make the busy cursor the current cursor.
*/
void XuSetCursor(Widget w, XuCURSOR_TYPE cursor_id, const Boolean state)
{
	int i, posn;
	Boolean compact;
	Cursor cursor;
	TOSET *tmp;

	if(!w) 
	{
		(void) fprintf(stderr,"XuSetCursor: NULL Widget passed in.\n");
		return;
	}
	if(XmIsGadget(w))
	{
		(void) fprintf(stderr,"XuSetCursor: Attempt to set cursor on Gadget \"%s\"\n", XtName(w));
		return;
	}
	if(!XtIsRealized(w))
	{
		tmp = XtNew(TOSET);
		tmp->w         = w;
		tmp->cursor_id = cursor_id;
		tmp->state     = state;
		tmp->count     = 0;
		(void)XtAppAddTimeOut(XtWidgetToApplicationContext(w), 1000,
						(XtTimerCallbackProc)DelayedCursorSetCB, (XtPointer)tmp);
		return;
	}

#ifdef PRINT_DEBUG
{	
/* Names for debug purposes */
static String cursor_names[] = {"Busy","Obscured","Stop","Finger","Knife","Magnify","Pan",
							"Pencil","Sample","ValueMod","Default","Menu"};
printf(">>>>> Cursor: %s  %s\n", cursor_names[cursor_id], state?"ON":"OFF");	
}
#endif

	if(state)
	{
		for(posn = 0; posn < history_len; posn++)
		{
			if(w == history[posn].w) break;
		}
		if(posn >= history_len)
		{
			for( posn = 0; posn < history_len; posn++ )
			{
				if(!history[posn].w) break;
			}
			if(posn == history_len)
			{
				history_len++;
				history = (CURHIST *)XtRealloc((void *)history,history_len*sizeof(CURHIST));
			}
			history[posn].w  = w;
			history[posn].dp = DefaultAppDisplayInfo;
			for(i = 1; i < Fxu.ndd; i++)
			{
				if( XtDisplayOfObject(history[posn].w) == Fxu.dd[i]->display )
				{
					history[posn].dp = Fxu.dd[i];
					break;
				}
			}
			for(i = 0; i < HIST_LEN; i++)
			{
				history[posn].clist[i] = GetCursor(posn, XuDEFAULT_CURSOR);
			}
			history[posn].obscured = False;
			history[posn].stop_sign = False;
			XtAddCallback(w, XmNdestroyCallback, PurgeCB, INT2PTR(posn));
		}
		if(cursor_id == XuWINDOW_OBSCURED_CURSOR)
		{
			history[posn].obscured = True;
		}
		else if(cursor_id == XuSTOP_CURSOR)
		{
			history[posn].stop_sign = True;
		}
		else
		{
			for( i =  HIST_LEN - 1; i > 0; i-- )
			{
				history[posn].clist[i] = history[posn].clist[i-1];
			}
			if(cursor_id >= XuBUSY_CURSOR && cursor_id <= XuVALUEMOD_CURSOR)
				history[posn].clist[0] = GetCursor(posn, cursor_id);
			else
				history[posn].clist[0] = GetCursor(posn, XuDEFAULT_CURSOR);
		}
	}
	else
	{
		posn = -1;
		for( i = 0; i < history_len; i++ )
		{
			if( w != history[i].w ) continue;
			posn = i;
			break;
		}
		if(posn < 0) return;

		if(cursor_id == XuWINDOW_OBSCURED_CURSOR)
		{
			history[posn].obscured = False;
		}
		else if(cursor_id == XuSTOP_CURSOR)
		{
			history[posn].stop_sign = False;
		}
		else
		{
			if(cursor_id < XuBUSY_CURSOR || cursor_id > XuVALUEMOD_CURSOR) return;
			compact = False;
			for(i = 0; i < HIST_LEN-1; i++)
			{
				if(history[posn].clist[i] == GetCursor(posn, cursor_id)) compact = True;
				if(!compact) continue;
				history[posn].clist[i] = history[posn].clist[i+1];
			}
			if(compact) history[posn].clist[HIST_LEN-1] = GetCursor(posn, XuDEFAULT_CURSOR);
		}
	}

	if(busy_cursor_count > 0)
		cursor = GetCursor(posn, XuBUSY_CURSOR);
	else if(history[posn].stop_sign)
		cursor = GetCursor(posn, XuSTOP_CURSOR);
	else if(history[posn].obscured)
		cursor = GetCursor(posn, XuWINDOW_OBSCURED_CURSOR);
	else
		cursor = history[posn].clist[0];

	XUndefineCursor(XtDisplayOfObject(w), XtWindowOfObject(w));
	XDefineCursor(XtDisplayOfObject(w), XtWindowOfObject(w), cursor);
}
