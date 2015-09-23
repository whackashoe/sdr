<?php
require('SDRDBClient.php');

$client = new SDRDBClient("unix:///tmp/sdrdb.sock");
$dbname = "test_" . mt_rand(1, 10000);
$width = 1000;
$amnt = 1000;

try {
    $client->create_database($dbname, $width);

    for($i=0; $i<$amnt; $i++) {
        $traits = [];
        for($j=0; $j<100; $j++) {
            $traits[] = mt_rand(1, $width - 1);
        }
        $client->put($dbname, $traits);
    }

    for($i=0; $i<10; $i++) {
        echo "similarity<br>";
        var_dump($client->query_similarity($dbname, mt_rand(1, $amnt), mt_rand(1, $amnt)));

        echo "closest<br>";
        var_dump($client->query_closest($dbname, 40, mt_rand(1, $width-1)));

        echo "matching<br>";
        var_dump($client->query_matching($dbname, [mt_rand(1, $width-1), mt_rand(1, $width-1)]));
    }
} catch(SDRDBException $e) {
    echo $e->getMessage();
}