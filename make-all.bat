@echo off
L:
cd \ALMA-FEControl\CppUtilityLib\Debug
make clean
make all
cd \ALMA-FEControl\FEICDBLib\Debug
make clean
make all
cd \ALMA-FEControl\FrontEndAMBLib\Debug
make clean
make all
cd \ALMA-FEControl\FrontEndAMBDLL\Debug
make clean
make all
cd \ALMA-FEControl\FrontEndControl2
make clean
make all
rem cd \ALMA-FEControl\FrontEndICDTest
rem make clean
rem make all
cd \ALMA-FEControl
