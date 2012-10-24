/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// main.cpp

#include "stdafx.h"
#include "Emulator.h"


void Test01_Basic10()
{
    Test_Init(_T("TEST 1: BASIC BK0010"), BK_CONF_BK0010_BASIC);

    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test01_01.bmp"), 1);

    Emulator_KeyboardSequence("PRINT PI\n");

    Emulator_KeyboardSequence("05 COLOR 2\n");
    Emulator_KeyboardSequence("10 FOR I=32 TO 127\n");
    Emulator_KeyboardSequence("20 PRINT CHR$(I);\n");
    Emulator_KeyboardSequence("30 IF I MOD 16 = 15 THEN PRINT\n");
    Emulator_KeyboardSequence("50 NEXT I\n");
    Emulator_KeyboardSequence("55 COLOR 3\n");
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(25);  // Wait 1 second
    Test_CheckScreenshot(_T("data\\test01_02.bmp"), 1);

    Emulator_KeyboardSequence("COLOR 1\n");
    Emulator_KeyboardSequence("CLS\n");
    Emulator_KeyboardSequence("1  !\"#$%&'()*+,-./\n");
    Emulator_KeyboardSequence("2 0123456789:;<=>?\n");
    Emulator_KeyboardSequence("3 @[\\]^_ `{|}~\n");
    Emulator_KeyboardSequence("4 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n");
    Emulator_KeyboardSequence("5 abcdefghijklmnopqrstuvwxyz\n");
    Test_SaveScreenshot(_T("test01_03.bmp"));

    // BASIC speed test by Sergey Frolov, see http://www.leningrad.su/calc/speed.php
    Emulator_Reset();
    Emulator_Run(50);
    Emulator_KeyboardSequence("4 FOR I = 1 TO 10\n");
    Emulator_KeyboardSequence("5 A = 1.0000001\n");
    Emulator_KeyboardSequence("10 B = A\n");
    Emulator_KeyboardSequence("15 FOR J = 1 TO 27\n");
    Emulator_KeyboardSequence("20 A = A * A\n");
    Emulator_KeyboardSequence("25 B = B ^ 2.01\n");
    Emulator_KeyboardSequence("30 NEXT J\n");
    Emulator_KeyboardSequence("35 NEXT I\n");
    Emulator_KeyboardSequence("40 PRINT A, B\n");
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(1084);
    //Test_SaveScreenshotSeria(_T("video\\test01_%04u.bmp"), 12, 1);
    Test_CheckScreenshot(_T("data\\test01_04.bmp"));

    Test_Done();
}

void Test02_Focal10()
{
    Test_Init(_T("TEST 2: Focal BK0010"), BK_CONF_BK0010_FOCAL);

    Emulator_Run(50);
    Emulator_KeyboardSequence("V\n");

    Test_CheckScreenshot(_T("data\\test02_01.bmp"));

    //// BASIC speed test by Sergey Frolov, see http://www.leningrad.su/calc/speed.php
    //Emulator_KeyboardSequence("01.04 F I=1,10; DO 2\n");
    //Emulator_KeyboardSequence("01.40 T A,B,!\n");
    //Emulator_KeyboardSequence("01.50 Q\n");
    //Emulator_KeyboardSequence("02.05 S A=1.0000001\n");
    //Emulator_KeyboardSequence("02.10 S B=A\n");
    //Emulator_KeyboardSequence("02.15 F J=1,27; DO 3\n");
    //Emulator_KeyboardSequence("03.20 S A=A*A\n");
    //Emulator_KeyboardSequence("03.25 S B=B^2.01\n");
    ////Emulator_KeyboardSequence("03.40 T J,A,B,!\n");
    //Emulator_KeyboardSequence("GO\n");
    //Emulator_Run(200);
    //Test_SaveScreenshot(_T("test02_02.bmp"));
    ////NOTE: ��������� �������� � ������� ������������ ��� J=26 -- ������, �� ������� ��������

    Test_Done();
}

void Test03_Tmos()
{
    Test_Init(_T("TEST 3: TMOS tests"), BK_CONF_BK0010_FDD);

    Emulator_Run(50);
    //Test_SaveScreenshot(_T("test03_01.bmp"));
    Test_LoadBin(_T("data\\791401.bin"));
    Emulator_AttachTeletypeBuffer();
    Emulator_KeyboardSequence("S1000\n");
    //Test_SaveScreenshot(_T("test03_02.bmp"));
    Emulator_Run(300);  // Wait while the test runs 3 times
    const char * teletype = Emulator_GetTeletypeBuffer();
    if (0 == strcmp(teletype, "\r\n\x0ek prohod\r\n\x0ek prohod\r\n\x0ek prohod"))
        Test_LogInfo(_T("Teletype check passed"));
    else
        Test_LogError(_T("Teletype check FAILED"));
    Emulator_DetachTeletypeBuffer();
    //Test_SaveScreenshot(_T("test03_03.bmp"));

    Test_Done();
}

void Test04_MSTD11()
{
    Test_Init(_T("TEST 4: BK0011M MSTD"), BK_CONF_BK0011);

    Emulator_Run(75);
    Emulator_KeyboardSequence("160100G");
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test04_01.bmp"), 1);  // Menu
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- start ROM test
    Emulator_Run(16 * 25);
    Test_CheckScreenshot(_T("data\\test04_03.bmp"), 1);  // ROM test results
    Emulator_KeyboardPressRelease(012);  // Enter -- exit the test
    Emulator_Run(25);
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- start palette test
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test04_07_00.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_01.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_02.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_03.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_04.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_05.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_06.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_07.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_08.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_09.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_10.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_11.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_12.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_13.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_14.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_15.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- exit the test
    Emulator_Run(25);
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- open help page
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test04_08.bmp"), 1);

    Test_Done();
}

int _tmain(int argc, _TCHAR* argv[])
{
    SYSTEMTIME timeFrom;  ::GetLocalTime(&timeFrom);
    Test_LogInfo(_T("Initialization..."));

    Test01_Basic10();
    Test02_Focal10();
    Test03_Tmos();
    Test04_MSTD11();

    Test_LogInfo(_T("Finalization..."));
    SYSTEMTIME timeTo;  ::GetLocalTime(&timeTo);
    FILETIME fileTimeFrom;
    SystemTimeToFileTime(&timeFrom, &fileTimeFrom);
    FILETIME fileTimeTo;
    SystemTimeToFileTime(&timeTo, &fileTimeTo);

    DWORD diff = fileTimeTo.dwLowDateTime - fileTimeFrom.dwLowDateTime;  // number of 100-nanosecond intervals
    Test_LogFormat('i', _T("Time spent: %.3f seconds"), (float)diff / 10000000.0);

    Test_LogSummary();

    return 0;
}
