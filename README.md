# Simple Search Engine

Два консольных приложения, реализующих [полнотекстовый поиск](https://en.wikipedia.org/wiki/Full-text_search) над файлами.

## Описание

### Индексирующая программа
Осуществляет подготовку [инвертированного индекса](https://en.wikipedia.org/wiki/Inverted_index), рекурсивно обходя все файлы в заданной директории. Первым аргументом командной строки принимает путь к индексируемой директории, вторым - куда нужно сохранить индекс. Использует [алгоритм LZW](https://ru.wikipedia.org/wiki/Алгоритм_Лемпеля_—_Зива_—_Велча) для сжатия.

### Поисковая программа
Осуществляет поиск по построенному индексу, поддерживает синтаксис запросов с "AND" и "OR". Ранжирует документы согласно [BM25](https://en.wikipedia.org/wiki/Okapi_BM25), в качестве результата выдает список файлов и номера строк, где встречается данное слово. 

