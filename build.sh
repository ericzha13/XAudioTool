

set -x
CurrentPath=$(pwd)
echo copy lib start

mkdir -p test_tool/test_win/x64/Release
mkdir -p test_tool/test_win/x64/Debug
cp lib/win64/*   test_tool/test_win/x64/Release
cp lib/win64_debug/*   test_tool/test_win/x64/Debug

echo copy lib end

echo end