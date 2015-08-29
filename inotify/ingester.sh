cd /home/airavata/oodt/cas-filemgr-0.9/bin
./filemgr-client --url http://localhost:9000 --operation --ingestProduct --productName $1 --productStructure Hierarchical --productTypeName GenericFile --metadataFile file:///home/airavata/oodt/inotify/meta.txt --refs file://$2
