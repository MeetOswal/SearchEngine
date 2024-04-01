How To Run the Project:
- The 'OG Datset' folder contains the datset ('0000.parquet', '0001.parquet', '0002.parquet') used for the project (which are deleted while submission)
- The 'dataframe_preprocessing.ipynb' file runs on the datset to create 2 files 'modified_0002.pkl' and 'modified_text_0002.pkl'. These 2 files contain documents and their vector embdings for clustering.
- The 3 Model files: 'KMeans.ipynb', 'GMMs.ipynb' and 'mini batch-KMeans.ipynb' files do the clustering of the pkl files into given number of clusters these clusterd/segmented documents are stored in 'trecFile' folder in form of .trec files.
- The 'index.cpp' file creates 'index', 'lexicon' and 'page table' for each cluster/shard and all 3 files afor each cluster is sored in different folders( 0 - N ).
- The 'test case creation.ipynb' file is used to create test queries to run on clusters. the queries are randomly selected from the OG datset. These quries are stored in a text file in 'tests' folder
- The 'query.cpp' file gets the quries from the test folder and run the quries on each cluster and after running on each qury in test set outputs a average recall score.
-'subindex' and 'intermediate' are temp folders used during subindex mering and sorting.