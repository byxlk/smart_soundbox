/* Copyright Statement:
 *
 * (C) 2005-2016 MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its
 * licensors. Without the prior written permission of MediaTek and/or its
 * licensors, any reproduction, modification, use or disclosure of MediaTek
 * Software, and information contained herein, in whole or in part, shall be
 * strictly prohibited. You may only use, reproduce, modify, or distribute
 * (as applicable) MediaTek Software if you have agreed to and been bound by
 * the applicable license agreement with MediaTek ("License Agreement") and
 * been granted explicit permission to do so within the License Agreement
 * ("Permitted User"). If you are not a Permitted User, please cease any
 * access or use of MediaTek Software immediately.
 */

GDI resource generator readme

Overview
     This readme describes the usage of graphics, font engine and resource implementation
     with GDI resource generator.
     Please follow the steps to import image and font resources to your application
     through the UI.

Hardware and software environment
    Supported platforms
      LinkIt 2523 HDK and 2533 HDK.
    Supported software
      LinkIt SDK v4.2.0 or later
    Third-party tools and software
      The GDI resource generator tool requires third-party tools - GCC, Make, ActivePerl and Python.
      The recommended versions are:
        - GCC v3.3.1
        - MAKE v3.79.1
        - ActivePerl v5.8.6
        - Python v2.5.1
      The tool package includes all tools except ActivePerl.
      Install ActivePerl (version - 5.8.6) in your PC before using the resource generator tool.

Prepare the image resource files
    1) Place the image source file under the folder "custom_resource\images" as an input to
       the resource generator tool.
       Example
         \\resgen_tool\custom_resource\images\sample.bmp

Prepare the font resource files
    1) Add font resource files in the font description file "custom_resource\font\res_gen_font.cpp".
       The API usage is as follows:
         AddFont(
           char *language_name,/* The language name in string format. */
           char *str_langid,   /* The language ID in the format of "*#Country Code#*". */
           char *str_code,     /* The language code. */
           char *pbdf_filepath,/* BDF file path. */
           U16 fontSize,       /* The bit "or" with any value of
                                  MCT_SMALL_FONT, MCT_MEDIUM_FONT and MCT_LARGE_FON. */
           bool is_fixed,      /* Whether the font is fixed width. */
           U8 is_dwidth        /* Whether the BDF file has dwidth value. Dwidth value is
                                  usually used for complex languages, such as Hindi. */
         );

       Example
         AddFont(
               ("English"), ("*#0044#"), ("en-US"),
                FONT_DATA_FILE_PATH(English.bdf),
                MCT_SMALL_FONT | MCT_MEDIUM_FONT | MCT_LARGE_FONT,
                0, 0);
       Note
           You can add more than one BDF files into the resource list.
           If there are two resource files that contain the same Unicode, the one added first
           will be shown.

    2) Place the font source file (in BDF format) under "custom_resource\font\".
       Example
          \\resgen_tool\custom_resource\font\MTKProprietaryFont14_.bdf

Run the resource generator tool
    1) Navigate to the resource generator tool and run the following command to
       complete the preparation.
       >make resgen

    2) The tool will prompt a message "Resource generates success!", to indicate
       successful operation.
       Otherwise please check the error log in the log folder "\\resgen_tool\log"
       to debug.

    3) The output files are in the folder "\\resgen_tool\output".

Integrate the output files from resource generator tool into your project
    1) Copy all output files from the folder "\\resgen_tool\output" to
       your project folder.
       Example output source and header files are:
           custom_image_data_resource.c
           custom_image_data_resource.h
           custom_image_map.c
           custom_image_resource.c
           custom_resource_def.h
           FontRes.c
           L_gMTKProprietaryFont_small.h

    2) Add the output source files (.c) to your project's makefile.
       Example
           C_FILES += $(APP_RES_PATH_SRC)/custom_image_data_resource.c
           C_FILES += $(APP_RES_PATH_SRC)/custom_image_map.c
           C_FILES += $(APP_RES_PATH_SRC)/custom_image_resource.c
           C_FILES += $(APP_RES_PATH_SRC)/FontRes.c

    3) Make sure all resource header files (.h) are in your project's include path.

Next, build and run your application.
Please refer to MediaTek LinkIt™ SDK v4 GCC Build Environment Guide.
