#!/bin/zsh
set -e
cd "${0:A:h}"
cmake -S native -B native/build -DCMAKE_PREFIX_PATH=/opt/homebrew -DLUMA_WITH_QT=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build native/build --target luma_qt_workbench -j4
exec native/build/bin/luma_qt_workbench --page 0
