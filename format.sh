find * -iname *.h -o -iname *.cpp -not -path "build/*" | xargs clang-format -i
