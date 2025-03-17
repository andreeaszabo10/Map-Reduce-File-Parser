Copyright Szabo Cristina-Andreea 2024-2025

# **File Processing with MapReduce Paradigm**

## **Overview**
This project implements a file processing system using the **MapReduce** paradigm, utilizing multithreading to distribute tasks among multiple threads. The program processes a list of text files, extracts words, and writes the results into output files based on the first letter of each word. It leverages **pthread** for concurrency, handling multiple mapper and reducer threads.

## **Features**
- **Parallel Processing**: Divides the work between multiple threads using **MapReduce**.
- **Thread-safe Operations**: Uses mutexes to ensure safe concurrent access to shared data structures.
- **Efficient Word Indexing**: Indexes words and their occurrences in the input files.
- **Output Files**: Generates output files where words are sorted alphabetically and by the frequency of their occurrence in files, organized by their first letter.

## **How It Works**
1. **Input Parsing**  
   The program first reads input from a given file which contains a list of text files to process. It assigns an index to each file and passes the information to mapper threads.

2. **Mapper Function**  
   Each mapper thread reads words from one file:
   - It processes each word by converting it to lowercase and removing non-alphabetical characters.
   - It then adds the word to a shared map (`word_idx`), where the key is the word and the value is a set of indices indicating the files in which the word appears.

3. **Reducer Function**  
   After all mappers finish processing, reducer threads are responsible for:
   - Sorting words first by the number of files they appear in (in descending order) and then alphabetically.
   - Writing the sorted words into output files, each corresponding to one letter of the alphabet (e.g., `a.txt`, `b.txt`, etc.).

4. **Synchronization**  
   - **Mutexes** are used to prevent multiple threads from accessing shared resources (like the word index) simultaneously.
   - **Barriers** are used to synchronize the mappers and reducers, ensuring all mappers complete their task before reducers begin processing.

## **Architecture**
- **Mapper Threads**: Each thread processes one file at a time, extracting and normalizing words, and updating a shared word index.
- **Reducer Threads**: After all mappers finish, the reducers process the accumulated word index and generate output files.
- **Mutexes**: Used for safe access to shared resources, ensuring that only one thread modifies data at any given time.
- **Barriers**: Ensures that all mappers have finished their work before the reducers start.
  
