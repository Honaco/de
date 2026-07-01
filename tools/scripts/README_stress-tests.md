#### Список скриптов

Для функционирования всех скриптов необходимо наличие Аккорда в ЭВМ и подключенного к нему модуля accord-le.

* *acle-gxm2-write-stress-test.sh* - нагрузочное тестирование записи на gxm2
* *acle-gx-write-stress-test.sh* - нагрузочное тестирование записи на gx
* *amdz-read-test.sh* - нагрузочное тестирование чтения данных АМДЗ
* *amdz-stress-test-rnd.sh* - нагрузочное тестирование генератора случайных чисел АМДЗ

#### Краткое описание
**acle-gxm2-write-stress-test.sh**
Скрипт пишет файл <FileToWrite> в память аккорда серии gxm2 в бесконечном цикле с помощью программы WriteProgram
(в нашем случае acle-write-sectors), где <PathToExecWriteProgram> - путь до этой программы (если программа лежит в одной директории со скриптом, то используется в виде ./acle-write-sectors)
`sudo ./acle-gxm2-write-stress-test.sh <fileToWrite>  <PathToExecWriteProgram>`

**acle-gx-write-stress-test.sh**
Скрипт пишет файл <FileToWrite> в память аккорда серии gx в бесконечном цикле с помощью программы WriteProgram
(в нашем случае acle-write-sectors), где <PathToExecWriteProgram> - путь до этой программы (если программа лежит в одной директории со скриптом, то используется в виде ./acle-write-sectors)
`sudo ./acle-gxm2-write-stress-test.sh <fileToWrite>  <PathToExecWriteProgram>`

**amdz-read-test.sh**
Создает в указанной директории <dirname> файлы с именами вида <номер_итерации>data<номер_прочитанного_блока>, где номер итерации - номер итерации чтения данных АМДЗ (нумерация с нуля). <blocksize> - размер блока в байтах.
`sudo ./amdz-read-test-gx.sh <blocksize> <dirname>`

**amdz-stress-test-rnd.sh**
Создает файлы со сгенерированными АМДЗ случайными числами в директории <PathToOutPutDir>. <numberOfBitesPerReq> -
размер случайного числа в байтах. <pathToRndUtil> - путь к программе, генерирующей случайные числа на АМДЗ (
в нашем случае acle-get-random, если лежит в одной директории со скриптом, то указывается как ./acle-get-random)
`sudo ./amdz-stress-test-rnd.sh <pathToRndUtil> <numberOfBitesPerReq> <pathToOuputDir>`
