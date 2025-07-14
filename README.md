# Libdither

Overview
--------
Libdither is a library for *black-and-white and color image dithering*, written in C (ANSI C99 standard).
Libdither has no external dependencies and should compile easily on most current systems (e.g. Windows, Linux, MacOS).
All you need is a C compiler.

<b>This version of libdither is a *Release Candidate*</b>: it has been tested and is working, but minor code cleanup is still required.

The black-and-white only libdither version can now be found in the [libdither_mono branch](https://github.com/robertkist/libdither/tree/libdither_mono). 

Features
--------

* Color ditherers: error diffusion, ordered dithering
* Mono ditherers: grid dithering, ordered dithering, error diffusion, variable error diffusion (Ostromoukhov, Zhou Fang),
  pattern dithering, Direct Binary Search (DBS), dot diffusion, Kacker and Allebach dithering, Riemersma dithering, thresholding
* Support for color comparison modes: LAB76, LAB94, LAB2000, sRGB, linear, HSV, luminance, Tetrapal
* Support for color reduction: Median-Cut, Wu, KD-Tree
* Support for images with transparent background
* Libdither works in linear color space
* Dither matrices and inputs can be easily extended without having to change the dither code itself
* no external dependencies on other libraries
* works with C and C++ projects
* supports Windows, Linux, MacOS (universal binary support Intel / Apple silicon), and possible others

License Changes and 3rd-Party Code
----------------------------------

Libdither MIT licensed, but the following code parts have their own permissive license:

- [kdtree](https://github.com/jtsiomb/kdtree) license: requires inclusion of license terms in source and binary distributions ([more](https://github.com/jtsiomb/kdtree?tab=License-1-ov-file))
- [uthash](https://github.com/troydhanson/uthash) license: requires inclusion of license terms in source distributions ([more](https://github.com/troydhanson/uthash/blob/master/LICENSE))

Examples Color
--------------

### Palettes:

<table>
<tr>
    <td><b>Original</b></td>
    <td><b>EGA palette (16 colors)<br>error diffusion</b></td>
    <td><b>C64 palette (16 colors)<br>error diffusion</b></td>
</tr><tr>
    <td><img src="extra/examples/bike_small.png" width=233 height=255></td>
    <td><img src="extra/examples/errordiff_dither_EGA.png" width=233 height=255></td>
    <td><img src="extra/examples/errordiff_dither_C64.png" width=233 height=255></td>
</tr><tr>
    <td><b>Pico8 palette (16 colors)<br>error diffusion</b></td>
    <td><b>EGA palette (16 colors)<br>void dispersed dots</b></td>
    <td><b>C64 palette (16 colors)<br>void dispersed dots</b></td>
</tr><tr>
    <td><img src="extra/examples/errordiff_dither_Pico8.png" width=233 height=255></td>
    <td><img src="extra/examples/ordered_void-dispersed-dots_dither_EGA.png" width=233 height=255></td>
    <td><img src="extra/examples/ordered_void-dispersed-dots_dither_C64.png" width=233 height=255></td>
</tr>
</table>

Note: Palettes with a good range of hues and brightness, which matches the source
image work best. If the overall palette is too dark, or too bright, the image, too,
will be darker or brighte.

### Color Quantization:

<table>
<tr>
    <td><b>Median-Cut: 16 colors<br>error diffusion</b></td>
    <td><b>Wu: 16 colors<br>error diffusion</b></td>
    <td><b>KD-Tree: 16 colors<br>error diffusion</b></td>
</tr><tr>
    <td><img src="extra/examples/errordiff_dither_mediancut.png" width=233 height=255></td>
    <td><img src="extra/examples/errordiff_dither_wu.png" width=233 height=255></td>
    <td><img src="extra/examples/errordiff_dither_kd.png" width=233 height=255></td>
</tr>
</table>

Note: Median-Cut and Wu are fairly fast, compared to KD-Tree. However, KD-Tree often gives
the best results.

### Color Reduction:

<table>
<tr>
    <td><b>Original</b></td>
    <td><b>Median-Cut: 2 colors<br>error diffusion</b></td>
    <td><b>Median-Cut: 4 colors<br>error diffusion</b></td>
</tr><tr>
    <td><img src="extra/examples/david.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_2.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_4.png" width=220 height=262></td>
</tr><tr>
    <td><b>Median-Cut: 8 colors<br>error diffusion</b></td>
    <td><b>Median-Cut: 16 colors<br>error diffusion</b></td>
    <td><b>Median-Cut: 32 colors<br>error diffusion</b></td>
</tr><tr>
    <td><img src="extra/examples/errordiff_dither_8.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_16.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_32.png" width=220 height=262></td>
</tr>
</table>

Note: With a palette that matches the original palette well, 32 colors are often sufficient.

### Color Matching / Color Distance function:

<table>
<tr>
    <td><b>LAB '76: 16 colors<br>error diffusion</b></td>
    <td><b>LAB '04: 16 colors<br>error diffusion</b></td>
    <td><b>LAB 2000: 16 colors<br>error diffusion</b></td>
</tr><tr>
    <td><img src="extra/examples/errordiff_dither_lab76.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_lab94.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_lab2000.png" width=220 height=262></td>
</tr><tr>
    <td><b>sRGB: 16 colors<br>error diffusion</b></td>
    <td><b>sRGB CCIR: 16 colors<br>error diffusion</b></td>
    <td><b>linear: 16 colors<br>error diffusion</b></td>
</tr><tr>
    <td><img src="extra/examples/errordiff_dither_srgb.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_srgb_ccir.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_linear.png" width=220 height=262></td>
</tr><tr>
    <td><b>linear: 16 colors<br>error diffusion</b></td>
    <td><b>linear CCIR: 16 colors<br>error diffusion</b></td>
    <td><b>HSV: 16 colors<br>error diffusion</b></td>
</tr><tr>
    <td><img src="extra/examples/errordiff_dither_linear.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_tetrapal.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_hsv.png" width=220 height=262></td>
</tr><tr>
    <td><b>luminance: 4 colors<br>error diffusion</b></td>
    <td><b>LAB 2000: 4 colors<br>error diffusion</b></td>
    <td><b>HSV: 4 colors<br>error diffusion</b></td>
</tr><tr>
    <td><img src="extra/examples/errordiff_dither_gb_luminance.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_gb_lab2000.png" width=220 height=262></td>
    <td><img src="extra/examples/errordiff_dither_gb_hsv.png" width=220 height=262></td>
</tr>
</table>

Notes:

- Different color distance measuring methods are supported for finding the closest color in the target palette.
- sRGB distance is a good, universally applicable choice.
- LAB2000 distance is the most accurate method. It works best with well balanced palettes, where it can pick up even subtle nuances, but it is slow.
- Luminance distance works best for gradients.
- CCIR distance methods take human perceptions into account and add a slight saturation boost.
- Other distance methods may work better than others in certain cases - make sure to try them.



Examples Mono
-------------

<table>
<tr>
    <td><b>Original</b></td>
    <td><b>Grid dither</b></td>
    <td><b>Xot error diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186177975-7fff4143-5e60-4270-b1c3-43f517445abd.png" width=220 height=262></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186177999-d243145a-a5cf-44fa-81b7-95b59ab571cf.png" width=220 height=262></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178037-48177c6e-96f3-4a4b-a21a-72ed0ad57b7d.png" width=220 height=262></td>
</tr><tr>
    <td><b>Diagonal error diffusion</b></td>
    <td><b>Floyd Steinberg error diffusion</td>
    <td><b>ShiauFan 3 error diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178045-5835631a-183b-473d-9925-fc43cb871c34.png" width=220 height=262></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178049-01f3b989-718a-47d5-bb9e-3b92d6987c58.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178053-72b48a34-24c2-49d1-989b-d13b6ebb310f.png"></td>
</tr><tr>
    <td><b>ShiauFan 2 error diffusion</b></td>
    <td><b>ShiauFan 1 error diffusion</b></td>
    <td><b>Stucki error diffusion</td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178056-7a36ec8f-a592-4a03-856d-b66c47b7ccd3.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178058-74d0ca18-f5a5-4d1d-afa1-d63711229b74.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178065-4e31cbd2-4ce3-4488-9fbf-977fc98d7ddc.png"></td>
</tr><tr>
    <td><b>1D error diffusion</b></td>
    <td><b>2D error diffusion</b></td>
    <td><b>Fake Floyd Steinberg error diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178070-d5c3ee60-ec13-4e69-af89-b880614e51e6.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178075-9550fdab-3246-4a4f-8616-b18845a2a871.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178079-b78ae193-7028-4512-a98e-e413d8b7c86f.png"></td>
</tr><tr>
    <td><b>Jarvis-Judice-Ninke error diffusion</td>
    <td><b>Atkinson error diffusion</b></td>
    <td><b>Burkes error diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178085-f22ed91e-f1d2-45fc-a7b0-b5e6ade6c6e8.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178086-1ed082fa-c55b-42a7-af59-39e4dd8751ad.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178093-9d87abd5-09c0-4c47-b83b-e31bfdea6b0f.png"></td>
</tr><tr>
    <td><b>Sierra 3 error diffusion</b></td>
    <td><b>Sierra 2-row error diffusion</td>
    <td><b>Sierra Lite error diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178097-36d03bce-316b-4f06-8a04-408074269075.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178103-e6c0e460-5721-4fc2-93e6-62103ddc8bbf.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178107-c13cf435-2696-42c4-bf36-46dadc188411.png"></td>
</tr><tr>
    <td><b>Steve Pigeon error diffusion</b></td>
    <td><b>Robert Kist error diffusion</b></td>
    <td><b>Stevenson Arce error diffusion</td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178108-5f84feb7-5882-4e9d-b7a0-f3bf4f27f989.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178111-af9630a4-6e5d-4b5f-b3a8-f1c6b7caa609.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186178113-24fc54bc-e15a-4e31-b67e-25a94442d279.png"></td>
</tr><tr>
    <td><b>Blue Noise dithering</b></td>
    <td><b>Bayer 2x2 ordered dithering</b></td>
    <td><b>Bayer 3x3 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179430-db09760f-fa60-4d29-9da8-fb03348d98ae.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179439-0a0dab3e-481d-461d-a6dd-ecd6bf97db68.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179442-60d12f26-b5a5-498b-849c-69b1e46a7f02.png"></td>
</tr><tr>
    <td><b>Bayer 4x4 ordered dithering</td>
    <td><b>Bayer 8x8 ordered dithering</b></td>
    <td><b>Bayer 16x16 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179444-e8eea4d0-1a5e-4f4e-a610-9211d664c8b3.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179446-e187f261-074e-4ddf-b110-59eeb569ecc8.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179447-5e099888-6bdc-4fec-9382-aeb0f669bd4a.png"></td>
</tr><tr>
    <td><b>Bayer 32x32 ordered dithering</b></td>
    <td><b>Dispersed Dots v1 ordered dithering</td>
    <td><b>Dispersed Dots v2 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179455-b78c7c07-74ba-41df-a187-2af3326bfbed.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179461-b9ddd9cb-59c3-4261-856e-477213ef8e51.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179466-fafb2de2-b34d-4847-826b-eb6e22646a08.png"></td>
</tr><tr>
    <td><b>Ulichney Void Dispersed Dots ordered dithering</b></td>
    <td><b>Non-Rectangular v1 ordered dithering</b></td>
    <td><b>Non-Rectangular v2 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179468-d5d64f66-5c5a-470d-acb3-75c75cf376ef.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179473-86ba710d-a8f4-4e3f-99aa-c690fbf159a4.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179479-74302bab-905b-4cd1-998e-41ad2880ad15.png"></td>
</tr><tr>
    <td><b>Non-Rectangular v3 ordered dithering</b></td>
    <td><b>Non-Rectangular v4 ordered dithering</b></td>
    <td><b>Ulichney Bayer 5x5 ordered dithering</td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179482-41937a9a-ad4c-4294-8cc8-368cb27f24c7.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179485-299b9d28-e0b5-4419-9b5c-41f06f56b4a9.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179490-dda11896-d568-4818-a350-c84b81c1484c.png"></td>
</tr><tr>
    <td><b>Ulichney ordered dithering</b></td>
    <td><b>Clustered Dot v1 ordered dithering</b></td>
    <td><b>Clustered Dot v2 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179499-27a14fb4-e4a1-4218-9671-a39ad2e19b57.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179494-894e4b60-fbbf-4f56-8f9c-9f1407f1895d.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179500-a7642aa2-dd03-437b-a4d2-9f7036bdf2b8.png"></td>
</tr><tr>
    <td><b>Clustered Dot v3 ordered dithering</b></td>
    <td><b>Clustered Dot v4 ordered dithering</b></td>
    <td><b>Clustered Dot v5 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179502-29bb0e04-9b88-40b0-9399-7160d302e373.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179508-3fd0d21d-4393-4526-a23b-0ddcf7b5df7b.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179512-8bbf7d9e-399d-40c8-a512-ef1f773a7ddc.png"></td>
</tr><tr>
    <td><b>Clustered Dot v6 ordered dithering</b></td>
    <td><b>Clustered Dot v7 ordered dithering</b></td>
    <td><b>Clustered Dot v8 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179513-d6e459e4-7428-4d3c-8c7c-7d43b815efd2.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179515-b3064791-5dc3-43f6-a310-be9c5a986f12.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179521-976a4191-ae6b-4c94-8206-808f697621fe.png"></td>
</tr><tr>
    <td><b>Clustered Dot v9 ordered dithering</b></td>
    <td><b>Clustered Dot v10 ordered dithering</b></td>
    <td><b>Clustered Dot v11 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179522-e9ac4c64-1f28-420d-9145-2db7a51dab12.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179529-bf2cc279-9c2a-4ee9-98bb-d09a6d009116.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179531-274a7c3d-f8f4-44c8-bc4f-5817a003e48c.png"></td>
</tr><tr>
    <td><b>Central White-Point ordered dithering</b></td>
    <td><b>Balanced Central White-Point ordered dithering</b></td>
    <td><b>Diagonal ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179533-16a96117-2f4d-4ecf-bcfb-40636702add9.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179536-acb20e48-332a-4c44-9367-7982cdaf6e9b.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179539-8c6017f5-7190-4d25-aadd-5f1e1030b8fa.png"></td>
</tr><tr>
    <td><b>Ulichney Clustered Dot ordered dithering</b></td>
    <td><b>ImageMagick 5x5 Circle ordered dithering</b></td>
    <td><b>ImageMagick 6x6 Circle ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179543-34d938cc-ce8d-459d-b9ce-eaaa2b49010d.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179545-14afa3d7-4800-41f0-a6c9-bbf79565ff61.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179549-5e584c8d-03d5-4951-bd9c-19dfae1e000e.png"></td>
</tr><tr>
    <td><b>ImageMagick 7x7 Circle ordered dithering</b></td>
    <td><b>ImageMagick 4x4 45-degree ordered dithering</b></td>
    <td><b>ImageMagick 6x6 45-degree ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179553-2093acbd-ef57-4f9c-8bbc-d66d99d496d3.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179555-b9324f88-9edd-4942-8c19-c6399a2e6651.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179562-2accb163-a920-49cc-95cc-4d8a8b9bf2dd.png"></td>
</tr><tr>
    <td><b>ImageMagick 8x8 45-degree ordered dithering</b></td>
    <td><b>ImageMagick 4x4 ordered dithering</b></td>
    <td><b>ImageMagick 6x6 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179565-028b6bc9-25a3-4e50-83a0-9183945dc5ad.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179567-d699adab-9549-4a36-8679-80889473c5c1.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179573-fed6314b-37ed-4d76-b996-6c4f3b9c4508.png"></td>
</tr><tr>
    <td><b>ImageMagick 8x8 ordered dithering</b></td>
    <td><b>Variable 2x2 ordered dithering</b></td>
    <td><b>Variable 4x4 ordered dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179576-07bf4815-7039-4256-b2bd-f7bc6dd9a290.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179581-0c01997c-32c1-4c59-a094-fbda2d6db43e.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179587-5add9952-1c5e-4520-924d-5eeb52944c60.png"></td>
</tr><tr>
    <td><b>Interleaved Gradient ordered dithering</b></td>
    <td><b>Ostromoukhov variable error diffusion</b></td>
    <td><b>Zhou Fang variable error diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186179590-0d025e87-efff-4283-8eac-fc89343beac3.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181357-6f6f178c-b99c-478b-8197-baf4ed64d3de.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181368-6a519417-8791-44d1-a226-6d0f2babdc09.png"></td>
</tr><tr>
    <td><b>Thresholding</b></td>
    <td><b>DBS v1 dithering</b></td>
    <td><b>DBS v2 dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181371-ae743639-1380-4aef-aefa-c8b0bde6aafe.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181373-b5185e9e-af7b-4897-a1e6-d81aa27f3508.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181376-76c11ff9-866c-41a1-beb6-9182229550ed.png"></td>
</tr><tr>
    <td><b>DBS v3 dithering</b></td>
    <td><b>DBS v4 dithering</b></td>
    <td><b>DBS v5 dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181384-2dac44b3-ee69-4e2a-a5d9-7d0f6efdc6ac.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181388-8123beed-4113-432c-a2ca-cbe8f4436537.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181393-a1a0ab58-2d8a-40bb-ba5d-126b37a21e02.png"></td>
</tr><tr>
    <td><b>DBS v6 dithering</b></td>
    <td><b>DBS v7 dithering</b></td>
    <td><b>Kacker and Allebach dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181398-eaa44b15-5b3c-44aa-89ed-eed39d0da427.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181402-6615dcfe-9efe-4bca-8128-0f0df36ac434.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181434-02904ef6-ab45-4fb3-bf74-20e074e7b413.png"></td>
</tr><tr>
    <td><b>Modified Riemersma (Hilbert Curve 1) dithering</b></td>
    <td><b>Modified Riemersma (Hilbert Curve 2) dithering</b></td>
    <td><b>Modified Riemersma (Peano Curve) dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186181440-ef8f2743-b348-45ef-a1fa-aadb6076664b.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294687-e9219209-c17d-4cb9-acd9-07f38578e2ec.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294737-fcd64796-d0b4-46c5-b84d-998a1c37f49d.png"></td>
</tr><tr>
    <td><b>Modified Riemersma (Fass0 Curve) dithering</b></td>
    <td><b>Modified Riemersma (Fass1 Curve) dithering</b></td>
    <td><b>Modified Riemersma (Fass2 Curve) dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294752-e7cde461-a720-4603-a14c-e980342ac388.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294758-2720dc4d-77ee-4370-a380-5ba0f9bcbf3d.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294764-3a0c3e6e-1afd-4933-932c-40f05c9a8221.png"></td>
</tr><tr>
    <td><b>Modified Riemersma (Gosper Curve) dithering</b></td>
    <td><b>Modified Riemersma (Fass Spiral) dithering</b></td>
    <td><b>Riemersma (Hilbert Curve 1) dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294769-0adc1780-08cc-4c3a-b408-1e1453b667cf.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294776-a238a175-17f1-4f32-bfdb-5528ba77b50e.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294784-e9f4225f-3e62-4fb3-918d-d9a0cf1e6292.png"></td>
</tr><tr>
    <td><b>Riemersma (Hilbert Curve 2) dithering</b></td>
    <td><b>Riemersma (Peano Curve) dithering</b></td>
    <td><b>Riemersma (Fass0 Curve) dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294789-c08a6db4-c793-4e1c-8c3d-8a0580d4780b.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294792-9664b245-0d7a-483a-8564-4b8beacb1a12.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294798-40460506-5eb9-470b-8644-82ff4d0e4516.png"></td>
</tr><tr>
    <td><b>Riemersma (Fass1 Curve) dithering</b></td>
    <td><b>Riemersma (Fass2 Curve) dithering</b></td>
    <td><b>Riemersma (Gosper Curve) dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294801-a84d203d-2dc0-4590-96de-9aec21ebf64c.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294805-f9b38cc7-c88f-4182-8991-b2f018c3513e.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294808-bd80cfa0-2ee9-411d-a0d8-9a01ca8f0f1a.png"></td>
</tr><tr>
    <td><b>Riemersma (Fass Spiral) dithering</b></td>
    <td><b>Pattern (2x2) dithering</b></td>
    <td><b>Pattern (3x3 v1) dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294815-5d2e7fd3-be51-449f-ab2b-c20977f5dc73.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294835-251988ab-a3ff-4c32-8f38-9d3013da8e5e.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294841-3507ab90-f192-4b75-b0dd-25b57c19734b.png"></td>
</tr><tr>
    <td><b>Pattern (3x3 v2) dithering</b></td>
    <td><b>Pattern (3x3 v3) dithering</b></td>
    <td><b>Pattern (4x4) dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294845-08fc6544-2299-4f16-925b-7c338a92539c.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294849-a55d5aa5-7251-41ae-a6c3-9358f5935189.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294854-b25ca41b-6e74-4770-9a58-22a7d670f2a9.png"></td>
</tr><tr>
    <td><b>Pattern (5x2) dithering</b></td>
    <td><b>Lippens and Philips v1 dot dithering</b></td>
    <td><b>Lippens and Philips v2 dot dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294857-b00f5d73-ffb2-40ff-b1e3-8daf6bfc1d33.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294860-36297c7c-a186-48f0-ac2f-e5883d67436a.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294864-95ff9e21-c6f5-4241-b1b8-ef7e5a35c460.png"></td>
</tr><tr>
    <td><b>Lippens and Philips v3 dot dithering</b></td>
    <td><b>Lippens and Philips (Guo Liu 16x16) dot dithering</b></td>
    <td><b>Lippens and Philips (Mese and Vaidyanathan 16x16) dot dithering</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294869-82444fbf-a81e-4aba-b3f5-135abffde96f.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294873-b4e12597-328a-404d-8149-6393270c1b7b.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294877-d7123015-a328-448f-a9b3-2018c640e1a1.png"></td>
</tr><tr>
    <td><b>Lippens and Philips (Knuth) dot dithering</b></td>
    <td><b>Knuth dot diffusion</b></td>
    <td><b>Mini-Knuth dot diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294880-7ed55bdc-b53f-4f92-962a-955a021c2f31.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294916-dca6d7d6-a83d-4ce5-b667-870d409e242d.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294928-0edb9314-d3ad-4a6c-a1e4-60944238f765.png"></td>
</tr><tr>
    <td><b>Optimized Knuth dot diffusion</b></td>
    <td><b>Mese and Vaidyanathan 8x8 diffusion</b></td>
    <td><b>Mese and Vaidyanathan 16x16 dot diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294936-19b563d1-c299-464a-9bfc-ff31ee621752.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294941-d7d3425f-52aa-4eb7-a9d5-92addefc6117.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294946-905eee7f-398d-4b8a-a19e-e9fd15c13ef0.png"></td>
</tr><tr>
    <td><b>Guo Liu 8x8 dot diffusion</b></td>
    <td><b>Guo Liu 16x16 dot diffusion</b></td>
    <td><b>Spiral dot diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294949-e955217f-95ce-4bda-91e8-66a40b57abf1.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294954-c692399a-2400-4978-863a-c229f34d7a0a.png"></td>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294956-6550812e-693c-47dc-aec8-63bf69d0f486.png"></td>
</tr><tr>
    <td><b>Inverted Spiral dot diffusion</b></td>
</tr><tr>
    <td><img src="https://user-images.githubusercontent.com/9162068/186294959-de3432b6-0961-4372-9afd-75d6b5f19f3d.png"></td>
</tr>
</table>

Building Libdither
------------------
You need an ANSI C compiler and the make utility. Run ```make``` to display all build options.
By default, libdither is built for the current architecture.
Once compiled, you can find the finished library in the ```dist``` directory.

*MacOS notes*:

* Installing the XCode command line tools is all you need for building libdither
* You can choose if you want to build a x64, arm64 or universal library. The demo, however, only builds against the current machine's architecture.

*Linux notes*:

* ```gcc``` and ```make``` is all you need to build libdither. E.g. on Ubuntu you should install build-essential via ```apt``` to get these tools.

*Windows notes*:

* You can build both MingW and MSVC targets from the Makefile (sorry, no .sln)
* For MingW, open the Makefile and ensure the path (on top of the file) points to your MingW installation directory
* Install make via Chocolatey package manager from chocolatey.org (https://chocolatey.org/, https://chocolatey.org/packages/make)

Usage
-----
The ```src/demo``` example shows how to load an image (we use .bmp as it's an easy format to work with),
convert it to linear color space, dither it, and write it back to an output .bmp file. The demo was used
to create all the dithering examples you can see below.

You can also look at ```libdither.h```, which includes commentary on how to use libdither.

In your own code, you only need to ```#include "libdither.h"```, which includes all public functions
and data structures, and link the libdither library, either statically or dynamically.
