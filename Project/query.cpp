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
#include <numeric>
#include <algorithm>
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::istringstream;
using std::map;
using std::ofstream;
using std::streampos;
using std::string;
using std::vector;
int shard = 0;
// BM25 score calculation function
// Function to calculate the BM25 score for a given term
// int spacePosition;
double calculateBM25(int termFrequency, int documentLength, int numberOfDocuments, int documentFrequency)
{
    const double k1 = 1.2;                 // Term saturation parameter
    const double b = 0.75;                 // Length normalization parameter
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
    // Construct the file name based on the shard value
    string fileName = std::to_string(shard) + "/lexicon_index.txt";
    // Open the file for reading
    std::ifstream inputFile(fileName);
    // std::ifstream inputFile("word_list.txt");
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Unable to open input file." << std::endl;
        return;
    }
    // Initialize variables for reading lines from the file
    std::string line;
    int LineNumber = 0;
    int startpoint = (*lexiconPointers)[word[0]]; // Get the startpoint based on the first character of the word
    // Loop through each line in the file
    while (std::getline(inputFile, line))
    {
        // Skip lines until reaching the starting point
        if (LineNumber != startpoint)
        {
            LineNumber++;
            continue;
        }
        // Break the loop if the first character of the line is different from the first character of the word
        if (line[0] != word[0])
        {
            break;
        }
        // Find the position of the ':' character in the line
        size_t delimiterPos = line.find(':');
        string key = line.substr(0, delimiterPos);
        // Check if the extracted key matches the target word
        if (word == key)
        {
            // Extract the value (portion after ':') from the line
            string value = line.substr(delimiterPos + 1);
            // Create a stringstream to parse the space-separated values in the 'value' string
            std::istringstream iss(value);
            string word;
            // Create a vector to store all the parsed values
            vector<string> all_values;
            // Loop through each word in the stringstream and add it to the vector
            while (iss >> value)
            {
                all_values.push_back(value);
            }
            // If the vector is not empty, update the wordMap with the key and its associated values
            if (!all_values.empty())
            {
                (*wordMap)[key] = all_values;
            }
        }
    }
    // Close the input file
    inputFile.close();
    // cout << "Lexicon Loaded" << endl;
    return;
}

void lexicon_fetch(map<char, int> *lexiconPointers)
{
    string fileName = std::to_string(shard) + "/lexicon_index.txt";
    std::ifstream inputFile(fileName);
    // std::ifstream inputFile("word_list.txt");
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Unable to open input file." << std::endl;
        return;
    }
    string line;
    char alphabet = ' ';
    int num = 0;
    // Vector to store pointers associated with each alphabet
    vector<int> pointers;
    // Loop through each line in the file
    while (std::getline(inputFile, line))
    {
        // Check if the first character of the current line is different from the current alphabet
        if (line[0] != alphabet)
        {
            // Find the position of the ':' character in the line
            size_t colonPos = line.find(':');
            // Extract the key (portion before ':') from the line
            string key = line.substr(0, colonPos);
            // Set the value to the current 'num'
            int value = num;
            // Update the lexiconPointers map with the current alphabet and its associated value
            (*lexiconPointers)[line[0]] = value;
            // Update the current alphabet
            alphabet = line[0];
        }
        // Increment 'num' for each line processed
        num++;
    }
    // Close the input file
    inputFile.close();
    // Return from the function after processing the file
    return;
}

// Function to load the mapping of document IDs to URLs from a file into a map
void fetchDocToURL(map<int, std::string> *docIDToURL)
{
    // Construct the file name based on the shard value
    string fileName = std::to_string(shard) + "/DocId.txt";
    // Open the file for reading
    std::ifstream inputFile(fileName);
    
    // Check if the file is successfully opened
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Unable to open the input file." << std::endl;
        return; // Exit the function if the file cannot be opened
    }
    // Initialize variables for reading lines from the file
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
int nextGEQ(size_t *startpointer, int k, size_t endpointer, int *SpacePosition)
{
    // Open the binary file
    string fileName = std::to_string(shard) + "/compressed_inverted_index.bin";
    ifstream binaryFile(fileName, std::ios::binary);
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
int getTermFrequency(int document, int startpointer, int endpointer, int *spacePosition)
{
    // Open the binary file
    string fileName = std::to_string(shard) + "/compressed_inverted_index.bin";
    ifstream binaryFile(fileName, std::ios::binary);
    // ifstream binaryFile("_inverted_index.txt");
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

double getScore(int startPointer, int endPointer, int d, map<int, std::string> *docIDToURL, int *spacePosition, int documentFrequency)
{
    // Calculate the term frequency of a term in a document
    int termFrequency = getTermFrequency(d, startPointer, endPointer, spacePosition); // Term frequency in the document
    // int termFrequency = 2.7;
    // Fetch the document length from the document ID to URL mapping
    int documentLength = getdocumentLength(docIDToURL, d);
    // Define constants and parameters for BM25 calculation
    int numberOfDocuments = 253344;
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
    int maxdocID = 253400;
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
    vector<int> documentfrequency = documentFrequency(wordMap, uniqueWords);
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

int main()
{
    // Initialize variables and containers for processing data
    string InputString;
    string inputString;
    vector<int> results;
    string filename = "tests/combined_output.txt"; //text file containing test quries
    ifstream file(filename);
    string line;
    double average = 0.00;
    int count = 0;
    int Line = 0;
    int SHARDS = 8;
    
// Check if the file is successfully opened
    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1; // Exit with an error code
    }
    // Loop through each line in the file
    while (std::getline(file, line))
    {
        // Extract the input string from the line
        InputString = line;
        // cout << line << endl;
        // Loop through each shard
        for (int i = 0; i < SHARDS; i++)
        {
            string inputString = InputString;
            shard = i;
            // Sample values
            // Initialize data structures
            map<string, vector<string>> wordMap;
            map<int, std::string> docIDToURL;
            vector<std::pair<int, double>> result;
            map<char, int> lexiconPointers;
            fetchDocToURL(&docIDToURL);
            lexicon_fetch(&lexiconPointers);

            vector<string> uniqueWords;

            // Tokenize the input string
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
            // Fetch lexicon files for each unique word
            for (string word : uniqueWords)
            {
                fetchLexiconFile(&wordMap, &lexiconPointers, word);
            }
            option = 1;  // Determine the option based on the existence of words in the wordMap
            for (string word : uniqueWords)
            {
                if (wordMap[word].empty())
                {
                    option = 2;
                    break;
                }
            }
            // Perform Conjunctive Search and update results
            switch (option)
            {
            case 1:
                result = ConjunctiveSearch(&wordMap, uniqueWords, &docIDToURL);

                if (result.empty())
                {
                    // cout << shard << " Total Results: 0" << endl;
                    results.push_back(0);
                    uniqueWords.clear();
                    wordMap.clear();
                }
                else
                {
                    // cout << shard << " Total Results: " << result.size() << endl;
                    results.push_back(result.size());
                }
                uniqueWords.clear();
                wordMap.clear();
                break;
            case 2:
                // cout << shard << " Total Results: 0" << endl;
                results.push_back(0);
                uniqueWords.clear();
                wordMap.clear();
                break;

            default:
                // cout << "Sorry Wrong Input" << endl;
                break;
            }
        }
        // Sort and calculate recall for the top 2 shards
                                                                //Change this term
       std::partial_sort(std::begin(results), std::begin(results) + 2, std::end(results), std::greater<int>());
        // For recall coverd by 2 shards
        int maxNumber = results[0] + results[1]; // For 2 Shards
        // int maxNumber = results[0]; //For 1 Shard
        int sum = std::accumulate(results.begin(), results.end(), 0);
        double recall = static_cast<double>(maxNumber) / sum;
        // Check for NaN to avoid division by zero
        if(!std::isnan(recall)){
            average += recall;
            count += 1;
        }
        // Clear results and update line count
        results.clear();
        Line += 1;
        // Print progress every 100 lines
        if (Line % 100 == 0) {
            cout << Line << endl;
        }
    }
    // Calculate and print the average recall
    average = average / count ;
    cout << "Average Recall: " << average;
    // Return from the program
    return 0;
}