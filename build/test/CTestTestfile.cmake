# CMake generated Testfile for 
# Source directory: /Users/toddnielsen/ECE_6785/2024_Lab3_Sameeran_Todd/test
# Build directory: /Users/toddnielsen/ECE_6785/2024_Lab3_Sameeran_Todd/build/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(simulate_mytest "/Applications/Renode.app/Contents/MacOS/macos_run.command" "--disable-xwt" "--port" "-2" "--pid-file" "renode.pid" "--console" "-e" "\$ELF=@/Users/toddnielsen/ECE_6785/2024_Lab3_Sameeran_Todd/build/test/mytest.elf; \$WORKING=@/Users/toddnielsen/ECE_6785/2024_Lab3_Sameeran_Todd; include @/Users/toddnielsen/ECE_6785/2024_Lab3_Sameeran_Todd/test/simulate.resc; start")
set_tests_properties(simulate_mytest PROPERTIES  _BACKTRACE_TRIPLES "/Users/toddnielsen/ECE_6785/2024_Lab3_Sameeran_Todd/test/CMakeLists.txt;39;add_test;/Users/toddnielsen/ECE_6785/2024_Lab3_Sameeran_Todd/test/CMakeLists.txt;0;")
