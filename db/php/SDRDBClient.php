<?php

require('SDRDBException.php');

class SDRDBClient
{
    protected $bindpath;
    protected $port;
    protected $fp;
    protected $ferrno;
    protected $ferrstr;
    protected $sopen;

    public function __construct($bindpath, $port = -1)
    {
        $this->bindpath = $bindpath;
        $this->port = $port;
        $this->opensock();
    }

    public function __destruct()
    {
        $this->closesock();
    }

    protected function opensock()
    {
        $this->fp = fsockopen($this->bindpath, $this->port, $this->ferrno, $this->ferrstr);
        $this->sopen = true;

        if($this->fp === false) {
            $this->sopen = false;
            throw new SDRDBException("couldn't bind to $bindpath");
        }
    }

    protected function closesock()
    {
        if($this->sopen) {
            fclose($this->fp);
            $this->sopen = false;
        }
    }

    protected function is_error($str)
    {
        return substr($str, 0, 4) == "ERR:";
    }

    protected function write_to_sock($data)
    {
        if(! $this->sopen) {
            $this->opensock();
        }

        fwrite($this->fp, $data, strlen($data));
    }

    protected function read_from_sock()
    {
        if(! $this->sopen) {
            $this->opensock();
        }

        $res = "";
        while(!feof($this->fp)) {
            $res .= fgets($this->fp);
        }

        $this->closesock();

        return $res;
    }

    public function create_database($dbname, $amount)
    {
        $this->write_to_sock("create $dbname $amount\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return (int) $res;
    }

    public function drop_database($dbname)
    {
        $this->write_to_sock("drop $dbname\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return (int) $res;
    }

    public function clear_database($dbname)
    {
        $this->write_to_sock("clear $dbname\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return (int) $res;
    }

    public function resize_database($dbname, $amount)
    {
        $this->write_to_sock("resize $dbname $amount\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return (int) $res;
    }

    public function put($dbname, array $traits)
    {
        $swrite = "put $dbname";
        foreach($traits as $trait) {
            $swrite .= " $trait";
        }
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return (int) $res;
    }

    public function update($dbname, $concept_id, array $traits)
    {
        $swrite = "update $dbname $concept_id";
        foreach($traits as $trait) {
            $swrite .= " $trait";
        }
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return (int) $res;
    }

    public function query_similarity($dbname, $concept_a_id, $concept_b_id)
    {
        $swrite = "query $dbname similarity $concept_a_id $concept_b_id";
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return (int) $res;
    }

    public function query_usimilarity($dbname, $concept_id, array $other_concept_ids)
    {
        $swrite = "query $dbname usimilarity $concept_id";
        foreach($other_concept_ids as $id) {
            $swrite .= " $id";
        }
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return (int) $res;
    }

    public function query_closest($dbname, $amount, $concept_id)
    {
        $swrite = "query $dbname closest $amount $concept_id";
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        $ret = [];
        $pieces = explode(' ', $res);
        foreach($pieces as $p) {
            $kv = explode(':', $p);
            $ret[(int) $kv[0]] = (int) $kv[1];
        }

        return $ret;
    }

    public function query_matching($dbname, array $traits)
    {
        $swrite = "query $dbname matching";
        foreach($traits as $trait) {
            $swrite .= " $trait";
        }
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        $ret = explode(' ', $res);
        foreach($ret as $k => $v) {
            $ret[$k] = (int) $ret[$k];
        }

        return $ret;
    }

    public function query_matchingx($dbname, $amount, array $traits)
    {
        $swrite = "query $dbname matchingx $amount";
        foreach($traits as $trait) {
            $swrite .= " $trait";
        }
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        $ret = explode(' ', $res);
        foreach($ret as $k => $v) {
            $ret[$k] = (int) $ret[$k];
        }

        return $ret;
    }
}
