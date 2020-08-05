# db_converter
Game archive packer/unpacker for the S.T.A.L.K.E.R. game series.  
Based on bardak's converter from xray_re-tools.

Supported operating systems: Linux x86_64

| File format         | 1114               | 2215               | 2945               | 2947RU             | 2947WW             | XDB                |
| :------------------ | :----------------: | :----------------: | :----------------: | :----------------: | :----------------: | :----------------: |
| Unpacking           | :grey_question:    | :grey_question:    | :grey_question:    | :grey_question:    | :grey_question:    | :heavy_check_mark: |
| Packing             | :grey_question:    | :grey_question:    | :grey_question:    | :grey_question:    | :grey_question:    | :heavy_check_mark: |
| Archive splittng    | :grey_question:    | :grey_question:    | :grey_question:    | :grey_question:    | :grey_question:    | :grey_question:    |

| Game                     | SoC                | CS                 | CoP                |
| :----------------------- | :----------------: | :----------------: | :----------------: |
| Repacked archive working | :grey_question:    | :grey_question:    | :grey_question:    |


## Build
```
git clone https://github.com/Masterkatze/db_converter.git
cd db_converter
mkdir build
cd build
cmake ..
make
```


## PKGBUILD for Archlinux
https://gist.github.com/Masterkatze/17bb699edd85d848d6fe8e8956e18aa0
