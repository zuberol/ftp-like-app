# ftp-like-app
This app allows to upload data across remote machines.

# Co dziala
- Serwer nadaje co SENDRATE pakiety na multicast z plikami z katalogu roota.
- Klient odbiera multicast i wyswietla dostepne pliki na serwerze, ktore moze pobrac.
- Klient i serwer na innym porcie tworza sesje TCP i serwer przesyła 'temps.txt' klientowi (mozliwe, ze trzeba najpierw stworzyc 'temps.txt').



# Todo
- Dorobic switcha w kliencie i podpiąć reszte komend, tak jak w którymś z linków jest pokazane.
- Serwer odpala się na roocie.
- Format tlv miedzy przesylanymi plikami gdzies trzeba wcisnac. Mozna w multicascie jak serwer rozsyła liste z plikami.
- Wyczyscic kod i essa zdane, czy cos jeszcze moze dorobic trzeba?

## Konspekt
https://docs.google.com/document/d/1Z7IymtaWE6T5qg_rLOqmrj8BJSl6ZnI8_beYwrp4uRU/edit?usp=sharing

# Linki

https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/
https://clinuxcode.blogspot.com/2014/02/concurrent-server-handling-multiple.html
https://programmer.help/blogs/a-simple-ftp-client-implemented-in-c-language.html
https://www.armi.in/wiki/FTP_Server_and_Client_Implementation_in_C/C%2B%2B


