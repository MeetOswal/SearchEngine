#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <list>
#include <vector>
#include <map>
#include <chrono>
#include <filesystem>
#include <cstdio>
#include <cmath>
#include <windows.h>
#include <psapi.h>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::list;
using std::map;
using std::ofstream;
using std::streampos;
using std::string;
using std::unordered_map;
using std::vector;
// function is responsible for building the inverted index, a critical data structure
// in information retrieval.
// Function responsible for building the inverted index.
void createIndex(unordered_map<string, list<std::pair<int, int>>> *Index, vector<string> words, int id)
{
    // Iterate through the words in the document.
    for (string &w : words)
    {
        // Convert characters to lowercase.
        for (char &c : w)
        {
            c = std::tolower(c);
        }
        // Retrieve or create the entry for the word in the index
        auto &wordEntry = (*Index)[w]; // Create or retrieve the entry
        bool found = false;
        // Check if the word is already in the document.
        for (auto &pair : wordEntry)
        {
            if (pair.first == id)
            {
                pair.second++; // Increment the frequency
                found = true;
                break;
            }
        }

        if (!found)
        {
            wordEntry.emplace_back(id, 1); // Add a new entry for the word in the document.
        }
    }
}
// The `writeIndex` function handles the writing of subindexes to text files
void writeIndex(string name, unordered_map<string, list<std::pair<int, int>>> *Index)
{
    string fileName = "subindex/" + name + ".txt";
    ofstream file(fileName, std::ios::app);

    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open file " << fileName << " for writing." << std::endl;
        return;
    }
    // Loop through the map and save its content to the file
    // e. It opens the file and iterates through the index. For each word in the index, it
    // writes the word, followed by a colon, and then the list of document ID-frequency pairs for that
    // word. The function appends this information to the file.
    for (const auto &pair : *Index)
    {
        file << pair.first << ":";
        for (const auto &entry : pair.second)
        {
            file << entry.first << "|" << entry.second << " ";
        }
        file << "\n";
    }
    // Close the file
    file.close();
    (*Index).clear();
}
// The `writeDocID` function manages the creation of a document ID file
void writeDocID(unordered_map<int, string> *DocID, int key, string value, int word_count, size_t start, size_t end)
{

    ofstream file("DocId.txt", std::ios::app);

    if (!file.is_open())
    {
        cerr << "Error: Unable to open file DocId.txt for writing." << endl;
        return;
    }

    // (*DocID)[key] = value;
    // The function opens the file and appends the
    // document ID and value in the specified format
    // Append the document ID and value in the specified format.
    file << key << ":" << word_count << " " << value << " " << start << " " << end << std::endl;

    file.close();
}
// Remove non-alphabetic and non-space characters from a string.
string removeSymbols(const std::string &line)
{
    std::string result;
    for (char c : line)
    {
        if (std::isalpha(c) || std::isspace(c))
        {
            result += c;
        }else{
            result += " ";
        }
    }
    return result;
}
// Read a file and build an inverted index.
int readFile(string fileName, unordered_map<int, string> *DocID)
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
    SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;
    SIZE_T physMemUsed;

    ifstream file(fileName);
    if (!file.is_open())
    {
        cerr << "Error: Unable to open file " << fileName << endl;
        return 0;
    }

    int docID = 0;
    int count = 0;
    string line;
    bool url = false;
    bool text = false;
    string URL;
    unordered_map<string, list<std::pair<int, int>>> Index;
    size_t memoryUsage;
    size_t LIMIT = 1000 * 1048576;
    int word_count = 0;
    size_t start;
    size_t end;
    while (std::getline(file, line))
    {
        // Check for the end of the TEXT section.
        if (line.find("</TEXT>") != string::npos)
        {
            end = file.tellg();
            writeDocID(DocID, docID, URL, word_count, start, end);
            word_count = 0;
            URL.clear();
            text = false;
            physMemUsed = pmc.WorkingSetSize; // check memory usage // Check if it's time to write the current index to a file.
            if (docID % 1000 == 0 || physMemUsed > LIMIT)
            {
                count++;
                writeIndex(std::to_string(count), &Index);
            }
        }

        if (text)
        {
            vector<string> words;
            line = removeSymbols(line);
            std::istringstream iss(line);
            std::string word;

            while (iss >> word)
            {
                word_count++;
                words.push_back(word);
            }
            createIndex(&Index, words, docID);
        }

        if (url)
        {
            URL = line;
            url = false;
            text = true;
            start = file.tellg();
        }

        if (line.find("<TEXT>") != string::npos)
        {
            url = true;
            docID++;
        }
    }
    // Write the final index.
    count++;
    writeIndex(std::to_string(count), &Index);

    file.close();
    return count;
}
// The `mergeFiles` function is pivotal for merging subindexes and creating intermediate indexes.
void mergeFiles(const vector<string> &filenames, const string &outputFilename)
{

    map<string, vector<string>> mergedData;

    for (const string &filename : filenames)
    {
        ifstream file(filename);
        if (!file.is_open())
        {
            cerr << "Error: Unable to open file " << filename << endl;
            continue;
        }

        string line;
        // The `mergeFiles` function takes a vector of filenames representing subindexes
        // and an output filename for the intermediate index
        while (std::getline(file, line))
        {
            size_t colonPos = line.find(':');
            if (colonPos != string::npos)
            {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);

                mergedData[key].push_back(value);
            }
        }
        file.close();
        remove(filename.c_str());
    }

    ofstream outputFile(outputFilename);
    if (!outputFile.is_open())
    {
        cerr << "Error: Unable to open output file " << outputFilename << endl;
        return;
    }

    for (const auto &pair : mergedData)
    {
        outputFile << pair.first << ":";
        for (const std::string &value : pair.second)
        {
            outputFile << value << " ";
        }
        outputFile << std::endl;
    }

    outputFile.close();
}
// The `mergeSort` function takes two filenames representing intermediate indexes,
// sorts their data, and writes the merged data to an output file
void mergeSort(string filename1, string filename2, string outputfilename)
{

    ifstream file1(filename1);
    ifstream file2(filename2);

    ofstream outputFile(outputfilename);
    string line1, line2;
    bool fileTrue1 = true, fileTrue2 = true;
    bool success1, success2;
    while (true)
    {
        if (fileTrue1)
        {
            success1 = (bool)std::getline(file1, line1);
            fileTrue1 = false;
        }

        if (fileTrue2)
        {
            success2 = (bool)std::getline(file2, line2);
            fileTrue2 = false;
        }

        if (!success1 && !success2)
        {
            break;
        }
        else if (success1 && success2)
        {
            size_t colonPos1 = line1.find(':');
            string key1 = line1.substr(0, colonPos1);
            string value1 = line1.substr(colonPos1 + 1);

            size_t colonPos2 = line2.find(':');
            string key2 = line2.substr(0, colonPos2);
            string value2 = line2.substr(colonPos2 + 1);

            int comparisonResult = key1.compare(key2);

            if (comparisonResult < 0)
            {
                outputFile << key1 << ":" << value1 << "\n";
                fileTrue1 = true;
            }
            else if (comparisonResult > 0)
            {
                outputFile << key2 << ":" << value2 << "\n";
                fileTrue2 = true;
            }
            else
            {
                outputFile << key1 << ":" << value1 << value2 << "\n";
                fileTrue1 = true;
                fileTrue2 = true;
            }
           
        }
        else if(success1 && !success2){

            size_t colonPos1 = line1.find(':');
            string key1 = line1.substr(0, colonPos1);
            string value1 = line1.substr(colonPos1 + 1);
            outputFile << key1 << ":" << value1 << "\n";
            fileTrue1 = true;
        }
        else if(!success1 && success2){
            size_t colonPos2 = line2.find(':');
            string key2 = line2.substr(0, colonPos2);
            string value2 = line2.substr(colonPos2 + 1);
            outputFile << key2 << ":" << value2 << "\n";
            fileTrue2 = true;
        }

    }
    file1.close();
    file2.close();
    outputFile.close();
    remove(filename1.c_str());
    remove(filename2.c_str());
}
// The `mergeSort` function, along with its counterpart `mergeSortloop`, handles the sorting and
// merging of intermediate indexes
int mergeSortloop(int batchSize, int totalFiles, string openFrom, string writeTo)
{
    // The `mergeSortloop` function orchestrates the iterative merge
    // process. It takes batch size and total file count parameters, initiating multiple rounds of merging.
    // It facilitates the progressive creation of larger intermediate indexes.
    int intermediateTotal = 0;
    for (int batchStart = 0; batchStart < totalFiles; batchStart += batchSize)
    {
        vector<string> fileBatch;

        for (int i = batchStart; i < batchStart + batchSize && i <= totalFiles; i++)
        {
            fileBatch.push_back(openFrom + std::to_string(i) + ".txt");
        }

        string intermediateFile = writeTo + std::to_string(intermediateTotal) + ".txt";
        mergeSort(fileBatch[0], fileBatch[1], intermediateFile);
        intermediateTotal++;
    }

    return intermediateTotal;
}

std::vector<unsigned char> encodeVByte(uint32_t num)
{
    vector<unsigned char> bytes;
    // Reserving approximate space in the byte vector
    /* While the number is greater than 127, push the lower 7 bits of the number into
     bytes and right shift the number by 7 bits */
    while (num > 127)
    {
        bytes.push_back(static_cast<unsigned char>(num & 127));
        num = num >> 7;
    }

    // Push the remaining bits of the number into bytes, adding 128 to set the most significant bit to 1
    bytes.push_back(static_cast<unsigned char>(num + 128));

    return bytes;
}

void compress(std::string directory)
{
    // Encode an integer using VByte compression
    // Open the input file for reading.
    std::ifstream file(directory + "0.txt");
    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open file "
                  << "fileName" << std::endl;
        return;
    }
    // Open two output files: one for compressed data and one for the lexicon
    ofstream outFile("compressed_inverted_index.bin", std::ios::binary);
    ofstream outFileLexicon("lexicon_index.txt", std::ios::app);
    // std::string line = "aaas:  476|1  926|1   1002|1   2905|1";
    // Initialize variables and containers for data processing.
    string line;
    std::stringstream compressedString;
    vector<string> compressData;
    vector<int> docIds_list;
    vector<int> frequency_list;
    int lineNumber = 0;
    streampos startPosition;
    streampos endPosition;
    streampos midPosition;
    int docFrequency = 0;
    // Read each line from the input file
    while (std::getline(file, line))
    {
        startPosition = outFile.tellp();

        // Find the position of the colon in the line.
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            // Extract the key and value parts from the line.
            string key = line.substr(0, colonPos);
            string value = line.substr(colonPos + 1, line.length() + 1);
            
            if(key.length() > 30){
                continue;
            }

            int docID;
            int frequecy;
            string result;
            // Process each character in the value part.
            for (char c : line)
            {
                if (std::isdigit(c))
                {
                    result += c;
                }
                else if (!std::isalnum(c) && !isspace(c))
                {
                    if (result.length() > 0)
                    {
                        docID = std::stoi(result);
                        // docIds_list.push_back(docID);
                        docFrequency++;
                        result.clear();
                    }
                }
                else if (isspace(c))
                {
                    if (result.length() > 0)
                    {
                        frequecy = std::stoi(result);
                        frequency_list.push_back(frequecy);
                        result.clear();
                        // Compress the frequency and docID using VByte encoding.
                        std::vector<unsigned char> compressedBytes_docID = encodeVByte(docID);

                        // Append the compressed data to the compressedString.
                        outFile.write(reinterpret_cast<const char *>(compressedBytes_docID.data()), compressedBytes_docID.size());
                        outFile << " ";
                    }
                }
            }
            // Write the compressed data to the output file and reset the compressedString.
            midPosition = outFile.tellp();
            for (size_t i = 0; i < frequency_list.size(); ++i)
            {
                outFile << frequency_list[i] << " ";
            }
            // outFile << "\n";
            endPosition = outFile.tellp();
            frequency_list.clear();

            // Write lexicon information to the lexicon output file.
            outFileLexicon << key << ":" << docFrequency << " " << startPosition << " " << endPosition << " " << midPosition << std::endl;
            docFrequency = 0;
        }
    }
    // Close the output and input files.
    outFile.close();
    file.close();
    outFileLexicon.close();

    return;
}

void structureChange(string directory){
    // Encode an integer using VByte compression
    // Open the input file for reading.
    std::ifstream file(directory + "0.txt");
    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open file "<< "fileName" << std::endl;
        return;
    }
    // Open two output files: one for compressed data and one for the lexicon
    ofstream outFile("_inverted_index.txt", std::ios::app);
    ofstream outFileLexicon("word_list.txt", std::ios::app);
    // std::string line = "aaas:  476|1  926|1   1002|1   2905|1";
    // Initialize variables and containers for data processing.
    string line;
    vector<string> frequency_list;
    int lineNumber = 0;
    std::streampos startPosition;
    std::streampos endPosition;
    std::streampos midPosition;
    int docFrequency = 0;
    // Read each line from the input file
    while (std::getline(file, line))
    {
        startPosition = outFile.tellp();
        // Find the position of the colon in the line.
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            // Extract the key and value parts from the line.
            string key = line.substr(0, colonPos);
            string value = line.substr(colonPos + 1, line.length() + 1);
            
            if(key.length() > 30){
                continue;
            }

            string docID;
            string frequecy;
            string result;
            // Process each character in the value part.
            for (char c : line)
            {
                if (std::isdigit(c))
                {
                    result += c;
                }
                else if (!std::isalnum(c) && !isspace(c))
                {
                    if (result.length() > 0)
                    {
                        docID = result;
                        // docIds_list.push_back(docID);
                        docFrequency++;
                        result.clear();
                    }
                }
                else if (isspace(c))
                {
                    if (result.length() > 0)
                    {
                        frequecy = result;
                        frequency_list.push_back(frequecy);
                        result.clear();

                        // Append the compressed data to the compressedString.
                        outFile << docID;
                        outFile << " ";
                    }
                }
            }
            // Write the compressed data to the output file and reset the compressedString.
            midPosition = outFile.tellp();
            for (size_t i = 0; i < frequency_list.size(); ++i)
            {
                outFile << frequency_list[i] << " ";
            }
            // outFile << "\n";
            endPosition = outFile.tellp();
            frequency_list.clear();

            // Write lexicon information to the lexicon output file.
            outFileLexicon << key << ":" << docFrequency << " " << startPosition << " " << endPosition << " " << midPosition << std::endl;
            docFrequency = 0;
        }
    }
    // Close the output and input files.
    outFile.close();
    file.close();
    outFileLexicon.close();

    return;
}

int main()
{

    string fileName = "trecFiles/cluster_0_output.trec";

    unordered_map<int, string> DocID;
    // create subindex
    int totalFiles = readFile(fileName, &DocID);

    // int totalFiles = 30;

    int batchSize = 0;
    if (totalFiles >= 100)
    {
        batchSize = 100; // batchSize is the number of submindex which will be included in the intermediate index
    }
    else
    {
        batchSize = totalFiles;
    }

    vector<string> filenames;
    int intermediateTotal = 0;

    // 3214
    for (int batchStart = 1; batchStart <= totalFiles; batchStart += batchSize)
    {
        vector<string> fileBatch;

        for (int i = batchStart; i < batchStart + batchSize && i <= totalFiles; i++)
        {
            fileBatch.push_back("subindex/" + std::to_string(i) + ".txt");
        }

        string intermediateFile = "intermediate/" + std::to_string(intermediateTotal) + ".txt";
        mergeFiles(fileBatch, intermediateFile);
        intermediateTotal++;
    }

    // 33

    int finalList = intermediateTotal;
    bool value = false;
    string directory;
    while (true)
    {
        if (value)
        {
            finalList = mergeSortloop(2, finalList, "subindex/", "intermediate/");
            if (finalList == 1)
            {
                directory = "intermediate/";
                break;
            }
            value = false;
        }
        else
        {
            finalList = mergeSortloop(2, finalList, "intermediate/", "subindex/");
            if (finalList == 1)
            {
                directory = "subindex/";
                break;
            }
            value = true;
        }
    }
    structureChange(directory);
    // structureChange("intermediate/");
    compress(directory);
    // compress("intermediate/");
    return 0;
}
