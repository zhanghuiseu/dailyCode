find ./common/encrypt -name *.h -or -name *.cpp -or -name *.hpp | xargs clang-format -i
find ./common/include -name *.h -or -name *.cpp -or -name *.hpp | xargs clang-format -i
find ./common/src -name *.h -or -name *.cpp -or -name *.hpp | xargs clang-format -i
find ./test/ -name *.h -or -name *.cpp -or -name *.hpp | xargs clang-format -i