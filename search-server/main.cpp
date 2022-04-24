#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <numeric>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{
                ComputeAverageRating(ratings),
                status
            });
    }

    template <typename Pred>
    vector<Document> FindTopDocuments(const string& raw_query, Pred pred) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, pred);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status](int, DocumentStatus document_status, int) {
            return status == document_status;
            });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const DocumentData& document_data = documents_.at(document_id);
                if (predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};
/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
template<typename T>
ostream& operator<<(ostream& os, const vector<T>& v) {
    os << "[";
    bool first = true;
    for (const auto& i : v) {
        if (!first)os << ", ";
        os << i;
        first = false;
    }
    os << "]";
    return os;
}

template<typename T>
ostream& operator<<(ostream& os, const set<T>& s) {
    os << "{";
    bool first = true;
    for (const auto& i : s) {
        if (!first)os << ", ";
        os << i;
        first = false;
    }
    os << "}";
    return os;
}

template<typename Key, typename Value>
ostream& operator<<(ostream& os, const pair<Key, Value>& p) {
    os << p.first << ": " << p.second;
    return os;
}

template<typename Key, typename Value>
ostream& operator<<(ostream& os, const map<Key, Value>& m) {
    os << "{";
    bool first = true;
    for (const auto& i : m) {
        if (!first)os << ", ";
        os << i;
        first = false;
    }
    os << "}";
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Func>
void RunTestImpl(Func func, const string& name_func) {
    func();
    cerr << name_func << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
bool operator==(const Document& lhs, const Document& rhs) {
    return lhs.id == rhs.id && lhs.rating == rhs.rating && abs(lhs.relevance - rhs.relevance) < EPSILON;
}

ostream& operator<<(ostream& os, const Document& doc) {
    os << doc.id << ' ' << doc.relevance << ' ' << doc.rating;
    return os;
}

ostream& operator<<(ostream& os, const DocumentStatus& status) {
    if (status == DocumentStatus::ACTUAL) os << "ACTUAL"s;
    else if (status == DocumentStatus::BANNED) os << "BANNED"s;
    else if (status == DocumentStatus::IRRELEVANT) os << "IRRELEVANT"s;
    else if (status == DocumentStatus::REMOVED) os << "REMOVED"s;
    else os << "Unknown status"s;
    return os;
}

void TestEmptyDatabase() {
    SearchServer server;
    ASSERT(server.GetDocumentCount() == 0);
}

void TestCountDocument() {
    const string content = "cat"s;
    const vector<int> ratings = { 2, 2 };
    SearchServer server;
    int count_documents = 0;
    for (int id = 1; id <= 10; ++id) {
        server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
        ++count_documents;
        ASSERT(server.GetDocumentCount() == count_documents);
    }
}

void TestFindTopDocumentsRelevanseSorting() {
    SearchServer server;
    server.AddDocument(0, "cat dog"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(1, "dog"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(2, "snake map snake"s, DocumentStatus::ACTUAL, { 1 });

    //one word found in a document consisting of one word
    vector<Document> expected = { { 0, (log(3) * 1.0 / 2), 1 } };
    ASSERT_EQUAL(server.FindTopDocuments("cat"s), expected);

    //a test for finding a word that is contained in several documents
    expected = { {1, log(3.0 / 2), 1}, {0, (log(3.0 / 2) * 1.0 / 2), 1} };
    ASSERT_EQUAL(server.FindTopDocuments("dog"s), expected);

    //the word is found twice in a three-word document
    expected = { {2, (log(3) * 2.0 / 3), 1} };
    ASSERT_EQUAL(server.FindTopDocuments("snake"s), expected);

    //multiple-word query
    expected = { {0, (log(3) * 1.0 / 2 + log(3.0 / 2) * 1.0 / 2), 1}, {2, (log(3) * 2.0 / 3), 1}, {1, (log(3.0 / 2)), 1} };
    ASSERT_EQUAL(server.FindTopDocuments("cat dog snake"s), expected);

    //checking that with different relevance and rating, the sorting will be by relevance
    SearchServer server2;
    server2.AddDocument(0, "cat dog"s, DocumentStatus::ACTUAL, { 1 });
    server2.AddDocument(1, "dog"s, DocumentStatus::ACTUAL, { 2 });
    server2.AddDocument(2, "snake map snake"s, DocumentStatus::ACTUAL, { 3 });
    expected = { {0, (log(3) * 1.0 / 2 + log(3.0 / 2) * 1.0 / 2), 1}, {2, (log(3) * 2.0 / 3), 3}, {1, (log(3.0 / 2)), 2} };
    ASSERT_EQUAL(server2.FindTopDocuments("cat dog snake"s), expected);
    SearchServer server3;
    server3.AddDocument(0, "cat dog"s, DocumentStatus::ACTUAL, { 3 });
    server3.AddDocument(1, "dog"s, DocumentStatus::ACTUAL, { 2 });
    server3.AddDocument(2, "snake map snake"s, DocumentStatus::ACTUAL, { 1 });
    expected = { {0, (log(3) * 1.0 / 2 + log(3.0 / 2) * 1.0 / 2), 3}, {2, (log(3) * 2.0 / 3), 1}, {1, (log(3.0 / 2)), 2} };
    ASSERT_EQUAL(server3.FindTopDocuments("cat dog snake"s), expected);
}

void TestAddDocument() {
    const string content = "cat in the city"s;
    const int doc_id = 42;
    const vector<int> ratings = { 1 };
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "dfjkgh kjfgh dfgh"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "itmgh urag dfgkjokg"s, DocumentStatus::ACTUAL, ratings);
    vector<Document> expected = { { 42, log(3) * 1.0 / 4, 1 } };
    ASSERT_EQUAL(server.FindTopDocuments("cat"s), expected);
    ASSERT_EQUAL(server.FindTopDocuments("city"s), expected);
}

void TestMinusWords() {
    const string content1 = "cat in the city"s;
    const int doc_id_1 = 42;
    const string content2 = "dog in the city"s;
    const int doc_id_2 = 50;
    const vector<int> ratings = { 1 };
    SearchServer server;
    server.AddDocument(doc_id_1, content1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_2, content2, DocumentStatus::ACTUAL, ratings);
    vector<Document> expected = { {50, 0.0, 1} };
    ASSERT_EQUAL(server.FindTopDocuments("-cat city"s), expected);
}

void TestMatchingDocuments() {
    const string content = "cat in the city"s;
    const int doc_id = 42;
    const vector<int> ratings = { 1, 2, 3 };
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    {
        auto [words, status] = server.MatchDocument("cat city dog"s, 42);
        vector<string> expected_words = { "cat"s, "city"s };
        DocumentStatus expected_status = DocumentStatus::ACTUAL;
        ASSERT_EQUAL(words, expected_words);
        ASSERT_EQUAL(status, expected_status);
    }
    {
        auto [words, status] = server.MatchDocument("-cat city dog"s, 42);
        DocumentStatus expected_status = DocumentStatus::ACTUAL;
        ASSERT(words.empty());
        ASSERT_EQUAL(status, expected_status);
    }
}

void TestCalculatingRating() {
    const string content = "cat in the city"s;
    const int doc_id = 42;
    vector<int> ratings = { 2, 2, 8, 12 };
    SearchServer server;
    auto AverageRating = [](const vector<int>& ratings) {
        if (ratings.empty())return 0;
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        //не юзал accumulate, потому как лень было проверять подключена ли в тестирующей программе библиотека numeric (урок Фреймворк и поисковая система).
        //оказалась не подключена :)
    };
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    auto documents = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(documents[0].rating, AverageRating(ratings));
    ratings = {};
    server.AddDocument(0, "dog"s, DocumentStatus::ACTUAL, ratings);
    auto documents_with_null_rating = server.FindTopDocuments("dog"s);
    ASSERT_EQUAL(documents_with_null_rating[0].rating, AverageRating(ratings));
    ratings = { 1, 1, 2 };
    server.AddDocument(1, "map"s, DocumentStatus::ACTUAL, ratings);
    auto documents3 = server.FindTopDocuments("map"s);
    ASSERT_EQUAL(documents3[0].rating, AverageRating(ratings));
    ratings = { 1, 1, 0, 1 };
    server.AddDocument(2, "snake"s, DocumentStatus::ACTUAL, ratings);
    auto documents4 = server.FindTopDocuments("snake"s);
    ASSERT_EQUAL(documents4[0].rating, AverageRating(ratings));
    ratings = { 1, 2, 1, 1 };
    server.AddDocument(3, "country"s, DocumentStatus::ACTUAL, ratings);
    auto documents5 = server.FindTopDocuments("country"s);
    ASSERT_EQUAL(documents5[0].rating, AverageRating(ratings));
}

void TestFindTopDocumentsWithUserPredicate() {
    SearchServer server;
    server.AddDocument(1, "cat"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(2, "cat"s, DocumentStatus::ACTUAL, { 2 });
    server.AddDocument(3, "cat"s, DocumentStatus::IRRELEVANT, { 3 });
    server.AddDocument(4, "cat"s, DocumentStatus::BANNED, { 4 });
    vector<Document> expected = {};
    {
        auto documents = server.FindTopDocuments("cat"s, [](int, DocumentStatus status, int) {return status == DocumentStatus::REMOVED; });
        ASSERT_EQUAL(documents, expected);
    }
    server.AddDocument(5, "cat"s, DocumentStatus::REMOVED, { 5 });
    expected = { {5, 0.0, 5} };
    {
        auto documents = server.FindTopDocuments("cat"s, [](int, DocumentStatus status, int) {return status == DocumentStatus::REMOVED; });
        ASSERT_EQUAL(documents, expected);
    }
    expected = { { 2, 0.0, 2 } };
    {
        auto documents = server.FindTopDocuments("cat"s, [](int id, DocumentStatus status, int) {return id > 1 && id < 5 && status == DocumentStatus::ACTUAL; });
        ASSERT_EQUAL(documents, expected);
    }
    expected = {};
    {
        auto documents = server.FindTopDocuments("cat"s, [](int id, DocumentStatus status, int) {return id > 3 && status == DocumentStatus::ACTUAL; });
        ASSERT_EQUAL(documents, expected);
    }
    expected = { {3, 0.0, 3}, {2, 0.0, 2}, {1, 0.0, 1} };
    {
        auto documents = server.FindTopDocuments("cat"s, [](int, DocumentStatus, int rating) {return rating < 4; });
        ASSERT_EQUAL(documents, expected);
    }
}

void TestFindTopDocumentsWithSettingStatus() {
    SearchServer server;
    server.AddDocument(1, "cat"s, DocumentStatus::ACTUAL, { 1 });
    server.AddDocument(2, "cat"s, DocumentStatus::ACTUAL, { 2 });
    server.AddDocument(3, "cat"s, DocumentStatus::IRRELEVANT, { 3 });
    server.AddDocument(4, "cat"s, DocumentStatus::BANNED, { 4 });
    vector<Document> expected = {};
    {
        auto documents = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(documents, expected);
    }
    server.AddDocument(5, "cat"s, DocumentStatus::REMOVED, { 5 });
    expected = { {5, 0.0, 5} };
    {
        auto documents = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(documents, expected);
    }
    server.AddDocument(6, "cat"s, DocumentStatus::IRRELEVANT, { 6 });
    expected = { {6, 0.0, 6}, {3, 0.0, 3} };
    {
        auto documents = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(documents, expected);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestEmptyDatabase);
    RUN_TEST(TestCountDocument);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestFindTopDocumentsRelevanseSorting);
    RUN_TEST(TestCalculatingRating);
    RUN_TEST(TestFindTopDocumentsWithUserPredicate);
    RUN_TEST(TestFindTopDocumentsWithSettingStatus);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}