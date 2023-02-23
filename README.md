# cpp-search-server
Финальный проект: поисковый сервер

Учебный проект поискового сервера, осуществляющего поиск среди текстов документов с возможностью указания стоп-слов (игнорируется сервером), минус-слов (документы с ними не учитываются в выдаче). Пример: имеются следующие документы
1. белый кот и модный ошейник 
2. пушистый кот пушистый хвост 
3. ухоженный пёс выразительные глаза 

По запросу **кот** найдутся документы 1 и 2. По запросу **кот -ошейник -пушистый** не найдется ничего, потому что -ошейник исключило документ 1, а -пушистый - документ 2.

Если в запросе нет плюс-слов, сервер не найдет ничего.\
Если одно и то же слово будет минус- и плюс-словом, оно считается минус-словом.\
Ранжирование результата считается по TF-IDF, при равенстве - по рейтингу документа.\
Методы поиска документов по запросу имеют последовательную и параллельную версии.

Используемый стандарт языка: c++17

Пример использования кода:
```C++
    SearchServer search_server("and with"s);
    int id = 0;
    for (const string& text : {
        "white cat and yellow hat"s, "curly cat curly tail"s, "nasty dog with big eyes"s, "nasty pigeon john"s
        }) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
```
Вывод:
```
ACTUAL by default:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
{ document_id = 1, relevance = 0.173287, rating = 2 }
{ document_id = 3, relevance = 0.173287, rating = 1 }
BANNED:
Even ids:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
```
