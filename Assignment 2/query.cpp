#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <map>
#include <fstream>
#include <cstring>
#include <windows.h>
#include <psapi.h>
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::istringstream;
using std::map;
using std::ofstream;
using std::string;
using std::vector;
using std::streampos;
// BM25 score calculation function
// Function to calculate the BM25 score for a given term
// int spacePosition;
double calculateBM25(int termFrequency, int documentLength, int numberOfDocuments, int documentFrequency)
{
    const double k1 = 1.2;                  // Term saturation parameter
    const double b = 0.75;                  // Length normalization parameter
    const double avgDocumentLength = 87.0; // Average document length (you may need to adjust this)

    // Calculate IDF (Inverse Document Frequency)
    double idf = std::log((numberOfDocuments - documentFrequency + 0.5) / (documentFrequency + 0.5) + 1.0);

    // Calculate BM25 score
    double score = idf * (termFrequency * (k1 + 1.0)) / (termFrequency + k1 * ((1.0 - b) + b * (documentLength / avgDocumentLength)));

    return score;
}

// Function to load the lexicon index from a file into a map
void fetchLexiconFile(map<string, vector<string>> *wordMap, map<char, int> *lexiconPointers, string word)
{
    std::ifstream inputFile("lexicon_index.txt");
    // std::ifstream inputFile("word_list.txt");
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Unable to open input file." << std::endl;
        return;
    }
    std::string line;
    int LineNumber = 0;
    int startpoint = (*lexiconPointers)[word[0]];
    while (std::getline(inputFile, line))
    {
        if (LineNumber != startpoint)
        {
            LineNumber++;
            continue;
        }
        if (line[0] != word[0])
        {
            break;
        }
        size_t delimiterPos = line.find(':');
        string key = line.substr(0, delimiterPos);
        if (word == key)
        {
            string value = line.substr(delimiterPos + 1);
            std::istringstream iss(value);
            string word;

            vector<string> all_values;
            while (iss >> value)
            {
                all_values.push_back(value);
            }
            if (!all_values.empty())
            {
                (*wordMap)[key] = all_values;
            }
        }
    }
    inputFile.close();
    // cout << "Lexicon Loaded" << endl;
    return;
}

void lexicon_fetch(map<char, int> *lexiconPointers)
{
    std::ifstream inputFile("lexicon_index.txt");
    // std::ifstream inputFile("word_list.txt");
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Unable to open input file." << std::endl;
        return;
    }
    string line;
    char alphabet = ' ';
    int num = 0;
    vector<int> pointers;
    while (std::getline(inputFile, line))
    {
        if (line[0] != alphabet)
        {
            size_t colonPos = line.find(':');
            string key = line.substr(0, colonPos);
            int value = num;
            (*lexiconPointers)[line[0]] = value;
            alphabet = line[0];
        }
        num++;
    }
    inputFile.close();
    cout << "Lexicon Loaded" << endl;
    return;
}

// Function to load the mapping of document IDs to URLs from a file into a map
void fetchDocToURL(map<int, std::string> *docIDToURL)
{
    std::ifstream inputFile("DocId.txt");
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Unable to open the input file." << std::endl;
        return;
    }

    string line;
    while (std::getline(inputFile, line))
    {
        // Parse the line into key and value using ':' as the delimiter
        size_t delimiterPos = line.find(':');
        if (delimiterPos != std::string::npos)
        {
            int key = std::stoi(line.substr(0, delimiterPos));
            std::string value = line.substr(delimiterPos + 1);

            // Insert key-value pair into the map
            (*docIDToURL)[key] = value;
        }
    }

    inputFile.close();
    cout << "URL Loaded" << endl;
    return;
}

// Function to open a list of postings for a set of words
vector<size_t> openList(map<string, vector<string>> *wordMap, vector<string> uniqueWords)
{
    vector<size_t> pointers;
    for (string word : uniqueWords)
    {
        if ((*wordMap).count(word) > 0)
        {
            std::istringstream restoreStream((*wordMap)[word][1]);
            size_t restoredPosition;
            restoreStream >> restoredPosition;
            pointers.push_back(restoredPosition);
        }
    }
    return pointers;
}

// Function to close a list of postings for a set of words
vector<size_t> closeList(map<string, vector<string>> *wordMap, vector<string> uniqueWords)
{
    vector<size_t> pointers;
    for (string word : uniqueWords)
    {
        std::istringstream restoreStream((*wordMap)[word][2]);
        size_t restoredPosition;
        restoreStream >> restoredPosition;
        pointers.push_back(restoredPosition);
    }

    return pointers;
}

// Function to obtain a list of middle positions for a set of words
vector<size_t> midList(map<string, vector<string>> *wordMap, vector<string> uniqueWords)
{
    vector<size_t> pointers;
    for (string word : uniqueWords)
    {
        std::istringstream restoreStream((*wordMap)[word][3]);
        size_t restoredPosition;
        restoreStream >> restoredPosition;
        pointers.push_back(restoredPosition);
    }

    return pointers;
}

// Function to decode a VByte-encoded integer from a vector of bytes
int decodeVByte(const std::vector<unsigned char> bytes)
{
    int val = 0;
    int shift = 0;
    int b;
    int index = 0;
    while (index < bytes.size() && (b = bytes[index++]) < 128)
    {
        val = val + (b << shift);
        shift += 7;
    }

    val = val + ((b - 128) << shift);
    return val;
}

// Function to find the next greater or equal value in a postings list
int nextGEQ(size_t* startpointer, int k, size_t endpointer, int* SpacePosition)
{
    // Open the binary file
    ifstream binaryFile("compressed_inverted_index.bin", std::ios::binary);
    // ifstream binaryFile("_inverted_index.txt");

    // ofstream outputFile("output.txt", std::ios::app);
    if (!binaryFile.is_open())
    {
        cerr << "Failed to open the binary file for reading." << std::endl;
        return 1;
    }
    std::vector<unsigned char> docID;
    string docid;
    size_t position = *startpointer;
    int spaceCount = 0;
    binaryFile.seekg(*startpointer, std::ios::beg);
    char c;
    int num = 0;
    while (binaryFile.get(c) && position < endpointer)
    {
        if (std::isspace(c))
        {
            unsigned int value = decodeVByte(docID);
            // int value = std::stoi(docid);
            if (value > 4000000)
            {
                continue;
            }
            if (value > num)
            {
                spaceCount++;
                num = value;
            }
            if (value >= k)
            {
                *SpacePosition += spaceCount; 
                *startpointer = binaryFile.tellg();
                binaryFile.close();
                return value;
            }
            docID.clear();
            // docid.clear();
        }
        else
        {
            docID.push_back(c);
            // docid += c;
        }

        position++;
    }
    binaryFile.close();
    return 4000000;
}

// Function to get the term frequency of a term in a document
int getTermFrequency(int document, int startpointer, int endpointer, int* spacePosition)
{
    // Open the binary file
    // ifstream binaryFile("compressed_inverted_index.bin", std::ios::binary);
    ifstream binaryFile("_inverted_index.txt");
    // ofstream outputFile("output.txt", std::ios::app);
    if (!binaryFile.is_open())
    {
        cerr << "Failed to open the binary file for reading." << std::endl;
        return 1;
    }

    std::vector<unsigned char> docID;
    int position = startpointer;
    binaryFile.seekg(startpointer, std::ios::beg);
    char c;
    string line;
    int count = 0;
    string digit;
    while (binaryFile.get(c) && position < endpointer)
    {
        if (isdigit(c))
        {
            digit += c;
        }
        if (isspace(c))
        {
            count++;
            if (*spacePosition == count)
            {
                binaryFile.close();
                return std::stoi(digit);
            }
            else
            {
                digit.clear();
            }
        }
        position++;
    }
    binaryFile.close();
    return 0;
}

// Function to get the document length from the docIDToURL mapping
int getdocumentLength(map<int, std::string> *docIDToURL, int docID)
{
    string value = (*docIDToURL)[docID];
    size_t delimiterPos = value.find(' ');
    string docLength = value.substr(0, delimiterPos);
    return std::stoi(docLength);
}

double getScore(int startPointer, int endPointer, int d, map<int, std::string> *docIDToURL , int* spacePosition, int documentFrequency)
{
    // Calculate the term frequency of a term in a document
    int termFrequency = getTermFrequency(d, startPointer, endPointer, spacePosition); // Term frequency in the document
    // int termFrequency = 2.7;
    // Fetch the document length from the document ID to URL mapping
    int documentLength = getdocumentLength(docIDToURL, d);
    // Define constants and parameters for BM25 calculation
    int numberOfDocuments = 3213835;
    // int documentFrequency = 57;
    double bm25Score = calculateBM25(termFrequency, documentLength, numberOfDocuments, documentFrequency);
    // Uncomment the line below to print the BM25 score for debugging
    // cout << bm25Score << endl;
    return bm25Score; // Return the calculated BM25 score for the document
}

vector<int> documentFrequency(map<string, vector<string>> *wordMap, vector<string> words)
{
    // Create a map to store the order of words based on their positions in the wordMap
    vector<int> documentFrequency;
    // Iterate through the input words
    for (string word : words)
    {
        // Check if the word exists in the wordMap (map of words to positions)
        if ((*wordMap).count(word) > 0)
        {
            // Assign the word's position (first position, [0]) as the order
            documentFrequency.push_back(std::stoi((*wordMap)[word][0]));
        }
    }
    return documentFrequency;
}

vector<std::pair<int, double>> ConjunctiveSearch(map<string, vector<string>> *wordMap, vector<string> uniqueWords, map<int, std::string> *docIDToURL)
{
    // Create a map to store document IDs along with their scores
    map<int, double> URLs;
    // Create vectors to store pointers for opening, closing, and mid positions
    vector<size_t> startPointer;
    vector<size_t> endPointer;
    vector<size_t> midPointer;
    // Define the maximum document ID to search
    int maxdocID = 3300000;
    // Check if the list of unique words is empty
    if (uniqueWords.empty())
    {
        // If empty, return an empty vector of key-value pairs
        vector<std::pair<int, double>> keyValuePairs;
        return keyValuePairs;
    }

    // Retrieve the starting pointers for the unique words
    startPointer = openList(wordMap, uniqueWords);
    // Check if the starting pointers match the number of unique words
    if (startPointer.size() != uniqueWords.size())
    {
        // If not, return an empty vector of key-value pairs
        vector<std::pair<int, double>> keyValuePairs;
        return keyValuePairs;
    }

    // Retrieve the closing and mid pointers for the unique words
    endPointer = closeList(wordMap, uniqueWords);
    midPointer = midList(wordMap, uniqueWords);
    vector<int> documentfrequency =  documentFrequency(wordMap, uniqueWords);
    // Get the number of unique words
    int num = startPointer.size();
    int spacePosition[num] = {0};
    // Initialize a variable to track the document ID
    int did = 0;
    // Initialize a variable to accumulate the score
    double score = 0;
    // Iterate through document IDs until reaching the maximum
    while (did < maxdocID)
    {
        // Find the next document ID greater than or equal to the first starting pointer
        did = nextGEQ(&startPointer[0], did, midPointer[0], &spacePosition[0]);
        // If the document ID exceeds the maximum, continue to the next iteration
        if (did > maxdocID)
        {
            continue;
        }
        int i = 1;
        // Check if the document IDs for other words match the current document ID
        if (num > 1)
        {
            for (i = 1; i < num && nextGEQ(&startPointer[i], did, midPointer[i], &spacePosition[i]) == did; i++)
                ;
        }
        if (i == num)
        {
            // If the document ID matches all unique words, calculate the score
            score = 0;
            for (int i = 0; i < num; i++)
            {

                score = score + getScore(midPointer[i], endPointer[i], did, docIDToURL, &spacePosition[i], documentfrequency[i]);
            }
            // Store the document ID and its score in the URLs map
            URLs[did] = score;
            // Move to the next document ID
            did++;
        }
        else
        {
            // If not all unique words match, find the next document ID for the word that didn't match
            did = nextGEQ(&startPointer[i], did, midPointer[i], &spacePosition[i]);
        }
    }
    // Check if the URLs map is empty
    if (URLs.empty())
    {
        // If empty, return an empty vector of key-value pairs
        vector<std::pair<int, double>> keyValuePairs;
        return keyValuePairs;
    }
    // Create a vector of key-value pairs from the URLs map
    vector<std::pair<int, double>> keyValuePairs(URLs.begin(), URLs.end());
    // Sort the vector by values in descending order
    std::sort(keyValuePairs.begin(), keyValuePairs.end(), [](const auto &a, const auto &b)
              { return a.second > b.second; });

    // Retrieve the top 10 key-value pairs
    int topN = 10;
    return keyValuePairs;
}

vector<std::pair<int, double>> DisjunctiveSearch(map<string, vector<string>> *wordMap, vector<string> uniqueWords, map<int, std::string> *docIDToURL)
{
    // Create a map to store document IDs along with their scores
    map<int, double> URLs;
    vector<size_t> startPointer;
    vector<size_t> endPointer;
    vector<size_t> midPointer;
    int maxdocID = 3300000;
    if (uniqueWords.empty())
    {
        vector<std::pair<int, double>> keyValuePairs;
        return keyValuePairs;
    }
    // Retrieve the starting pointers for the unique words
    startPointer = openList(wordMap, uniqueWords);
    // Check if the starting pointers match the number of unique words
    if (startPointer.size() != uniqueWords.size())
    {
        vector<std::pair<int, double>> keyValuePairs;
        return keyValuePairs;
    }
    // Retrieve the closing and mid pointers for the unique words
    endPointer = closeList(wordMap, uniqueWords);
    midPointer = midList(wordMap, uniqueWords);
    vector<int> documentfrequency =  documentFrequency(wordMap, uniqueWords);
    int num = startPointer.size();
    int spacePosition[num] = {0};
    int did = 0;
    double score = 0;
    // Loop through each unique word's posting list
    for (int i = 0; i < num; i++)
    {
        did = 0;
        // Iterate through document IDs until reaching the maximum
        while (did < maxdocID)
        {
            // Find the next document ID greater than or equal to the current starting pointer
            did = nextGEQ(&startPointer[i], did, midPointer[i], &spacePosition[i]);
            // If the document ID exceeds the maximum, continue to the next iteration
            if (did > maxdocID)
            {
                continue;
            }
            // Calculate the score for the current document ID and word
            score = getScore(midPointer[i], endPointer[i], did, docIDToURL, &spacePosition[i], documentfrequency[i]);
            // Check if the document ID is already in the URLs map
            if (URLs.count(did) > 0)
            {
                URLs[did] += score;
            }
            else
            {
                // If it doesn't exist, create a new entry with the score
                URLs[did] = score;
            }
            did++;
        }
        score = 0;
    }
    if (URLs.empty())
    {
        // If empty, return an empty vector of key-value pairs
        vector<std::pair<int, double>> keyValuePairs;
        return keyValuePairs;
    }
    vector<std::pair<int, double>> keyValuePairs(URLs.begin(), URLs.end());
    // Sort the vector by values in descending order
    std::sort(keyValuePairs.begin(), keyValuePairs.end(), [](const auto &a, const auto &b)
              { return a.second > b.second; });

    // Retrieve the top 10 key-value pairs
    return keyValuePairs;
}

string getURL(map<int, std::string> *docIDToURL, int docID)
{
    string line = (*docIDToURL)[docID];
    size_t delimiterPos = line.find(' ');
    string url = line.substr(delimiterPos + 1);
    delimiterPos = url.find(' ');
    line = url.substr(0, delimiterPos);
    return line;
}

vector<string> splitIntoSentences(std::streampos  start, std::streampos  end, vector<string> *words)
{
    // Create a vector to store sentences that match the specified criteria
    std::vector<std::string> sentences;
    // Open the file "msmarco-docs/fulldocs-new.trec"
    ifstream File("msmarco-docs/fulldocs-new.trec");
    // Set the file's read position to the specified 'start' offset from the beginning
    File.seekg(start, std::ios::beg);
    // Initialize a variable to keep track of the current read position
    // Initialize variables to store characters and sentences
    char c;
    string line;
    // Loop through the file character by character until 'end' is reached
    while (File.get(c) && File.tellg() < end)
    {
        c = std::tolower(c);
        line += c;
        if(line.length() >= 300){
            sentences.push_back(line);
            line.clear();
            File.close();
            return sentences;
        }
    }
    line.clear();
    File.close();
    return sentences;
}

vector<string> snippet(map<int, std::string> *docIDToURL, int docID, vector<string> words)
{

    string line = (*docIDToURL)[docID]; // doclength docurl docstart docend
    size_t delimiterPos = line.find(' ');
    string url = line.substr(delimiterPos + 1); // docurl docstart docend
    delimiterPos = url.find(' ');
    line = url.substr(delimiterPos + 1); // docstart docend
    delimiterPos = line.find(' ');
    std::streampos  passageStart = std::stod(line.substr(0, delimiterPos)) + 15; // docstrat
    std::streampos  passageEnd = std::stod(line.substr(delimiterPos + 1));  // docend
    vector<string> sentences = splitIntoSentences(passageStart, passageEnd, &words);
    return sentences;
}

int main()
{
    // Sample values
    map<string, vector<string>> wordMap;
    map<int, std::string> docIDToURL;
    vector<std::pair<int, double>> result;
    map<char, int> lexiconPointers;
    fetchDocToURL(&docIDToURL);
    lexicon_fetch(&lexiconPointers);
    // fetchLexiconFile(&wordMap, &lexiconPointers, word);
    string inputString;
    int topN = 10;
    vector<string> uniqueWords;

    while (true)
    {
        cout << "Enter a string: ";
        std::getline(cin, inputString);
        // inputString = "aa";
        if (inputString == "Done")
        {
            break;
        }

        istringstream iss(inputString);
        string word;

        while (iss >> word)
        {
            // Check if the word is not already in the vector
            if (std::find(uniqueWords.begin(), uniqueWords.end(), word) == uniqueWords.end())
            {
                uniqueWords.push_back(word);
            }
        }
        int option;
        for (string word : uniqueWords)
        {
            fetchLexiconFile(&wordMap, &lexiconPointers, word);
        }
        for(string word : uniqueWords){
            cout << word << ": " << wordMap[word][0] << endl;
        }
        cout << "Lexicon Lookup Complete" << endl;
        cout << "Enter type of Search 1 or 2: ";
        cin >> option;
        cin.ignore();
        cout << "********************************************************" << endl;
        // option = 1;
        switch (option)
        {
        case 1:
            result = ConjunctiveSearch(&wordMap, uniqueWords, &docIDToURL);

            if (result.empty())
            {
                cout << "No Search Result Found" << endl;
            }
            else
            {
                for (int i = 0; i < topN && i < result.size(); i++)
                {
                    cout << "--------------------------------------------------------" << endl;
                    std::cout << "URL: " << getURL(&docIDToURL, result[i].first) << "| Score: " << result[i].second << std::endl;
                    cout << endl;
                    vector<string> snippets = snippet(&docIDToURL, result[i].first, uniqueWords);
                    if (!snippets.empty())
                    {
                        for (string snippet : snippets)
                        {
                            cout << snippet << endl;
                        }
                    }else{
                        cout << "Snippet Not generated" << endl;
                    }
                    cout << endl;
                }
            }
            uniqueWords.clear();
            wordMap.clear();
            break;
        case 2:
            result = DisjunctiveSearch(&wordMap, uniqueWords, &docIDToURL);
            if (result.empty())
            {
                cout << "No Search Result Found" << endl;
            }
            else
            {
                for (int i = 0; i < topN && i < result.size(); i++)
                {
                    cout << "--------------------------------------------------------" << endl;
                    std::cout << "URL: " << getURL(&docIDToURL, result[i].first) << ", Score: " << result[i].second << std::endl;
                    cout << endl;
                    vector<string> snippets = snippet(&docIDToURL, result[i].first, uniqueWords);
                    if (!snippets.empty())
                    {
                        for (string snippet : snippets)
                        {
                            cout << snippet << endl;
                        }
                    }
                    else{
                        cout << "Snippet Not generated" << endl;
                    }
                    cout << endl;
                }
            }
            uniqueWords.clear();
            wordMap.clear();
            break;

        default:
            cout << "Sorry Wrong Input" << endl;
            break;
        }
    }

    return 0;
}