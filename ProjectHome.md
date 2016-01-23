The project MOVED to GitHub:
  * https://github.com/nzeemin/bkbtl
  * https://github.com/nzeemin/bkbtl-qt
  * https://github.com/nzeemin/bkbtl-testbench
  * https://github.com/nzeemin/bkbtl-doc


---


**BKBTL** -- **BK Back to Life!** -- is [BK0010/BK0011](http://en.wikipedia.org/wiki/Elektronika_BK) emulator.
The emulation project started on Nov. 14, 2009 and based on [UKNCBTL](http://code.google.com/p/ukncbtl/) code.
BK is soviet home computer based on 16-bit PDP-11 compatible processor K1801VM1.

There is three branches of the emulator, with different user interface but the same emulation core:
  * **BKBTL** -- Windows version, for Windows 2000/XP/Vista/7.
  * **QtBkBtl** is Qt based BKBTL branch, works under Windows, Linux and Mac OS X.
  * **SdlBkBtl** is SDL version of the emulator. Currently targetted to Dingoo A320 native OS.

Current status: Beta, under development.

Emulated:
  * BK-0010.01 and BK-0011M
  * CPU (still closer to 1801VM2)
  * Motherboard (partially)
  * Screen -- black and white mode, color mode, short mode, BK0011 color mode palettes
  * Keyboard (but mapped not all BK keys)
  * Reading from tape (WAV file), writing to tape (WAV file)
  * Sound
  * Joystick (numpad keys, external joystick)
  * Covox
  * Floppy drive (at least in BK11M configuration)
  * Programmable timer (partially)


---


**BKBTL** -- **BK Back to Life!** -- это проект эмуляции советского бытового компьютера [БК-0010/БК-0011](http://ru.wikipedia.org/wiki/БК), построенного на 16-разрядном процессоре К1801ВМ1, совместимом по системе команд с семейством PDP-11.
Проект начат 14 ноября 2009 года. Основан на коде проекта [UKNCBTL](http://code.google.com/p/ukncbtl/).

В проект входят три ветки, имеющие разный интерфейс, но использующее общее ядро эмуляции:
  * **BKBTL** -- Windows-версия. Написана под Win32 и требует поддержки Юникода, поэтому набор версий Windows -- 2000/2003/2008/XP/Vista/7.
  * **QtBkBtl** -- Qt-версия. Работает под Windows, Linux и Mac OS X. В Qt-версии нет поддержки звука, нет окна карты памяти, нет поддержки внешнего джойстика; в остальном возможности те же.
  * **SdlBkBtl** -- SDL-версия эмулятора. Пока нацелена только на устройство Dingoo A320, незакончена.

## Состояние эмулятора ##
Бета-версия. Многие игры пока не работают. Дисковод более-менее работает в конфигурации БК-0011М.

Поддерживаются конфигурации: БК-0010.01+Бейсик, БК-0010.01+Фокал+тесты, БК-0010.01+дисковод, БК-0011М+тесты, БК-0011М+дисковод.

Эмулируется:
  * БК-0010.01 и БК-0011М
  * процессор (тест 791401 проходит, тест 791404 НЕ проходит)
  * материнская плата (частично, тест памяти 791323 НЕ проходит)
  * экран -- черно-белый, цветной, усеченный режим, палитры цветного режима БК-0011
  * клавиатура -- маппинг PC-клавиатуры на БК-клавиатуру зависит от переключателя РУС/ЛАТ в БК (но размаплены не все клавиши)
  * чтение с магнитофона (из файла формата WAV), запись на магнитофон (в WAV-файл)
  * звук пьезодинамика БК
  * Covox
  * джойстик (клавиши NumPad, внешний джойстик)
  * ИРПС на регистрах 177560..0177566 (пока только передача данных в отладочное окно) -- используется для прогона тестов
  * дисковод (более-менее в конфигурации БК-0011М)

Планируется сделать:
  * сделать правильную систему прерываний процессора (пока сделано ближе к ВМ2)
  * прогон тестов 791404 и 791323, отладка работы машины на них
  * доделать маппинг клавиатуры
  * программируемый таймер (нужно доделать)
  * мышь
  * AY-3-8910

## Скриншот ##

![http://bkbtl.googlecode.com/svn/trunk/docs/screenshot/BKBTL-20110618-main.png](http://bkbtl.googlecode.com/svn/trunk/docs/screenshot/BKBTL-20110618-main.png)

![http://bkbtl.googlecode.com/svn/trunk/docs/screenshot/QtBkBtl-20100518-mac.png](http://bkbtl.googlecode.com/svn/trunk/docs/screenshot/QtBkBtl-20100518-mac.png)

![http://bkbtl.googlecode.com/svn/trunk/docs/screenshot/SdlBkSdl_20101212.jpg](http://bkbtl.googlecode.com/svn/trunk/docs/screenshot/SdlBkSdl_20101212.jpg)


---
