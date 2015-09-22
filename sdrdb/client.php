<?php

class SDRDBException extends Exception
{
    public function __construct($message, $code = 0, Exception $previous = null)
    {
        parent::__construct($message, $code, $previous);
    }

    public function __toString() {
        return __CLASS__ . ": [{$this->code}]: {$this->message}\n";
    }
}

class SDRDBClient
{
    protected $bindpath;
    protected $fp;
    protected $ferrno;
    protected $ferrstr;

    public function __construct($bindpath, $port = -1)
    {
        $this->bindpath = $bindpath;
        $this->fp = fsockopen($bindpath, $port, $this->ferrno, $this->ferrstr);

        if($this->fp === false) {
            throw new SDRDBException("couldn't bind to $bindpath");
        }
    }

    public function __destruct()
    {
        fclose($this->fp);
    }

    protected function is_error($str)
    {
        return substr($str, 0, 4) == "ERR:";
    }

    protected function write_to_sock($data)
    {
        fwrite($this->fp, $data);
    }

    protected function read_from_sock()
    {
        $res = "";
        while(!feof($this->fp)) {
            $res .= fgets($this->fp);
        }

        return $res;
    }

    public function create_database($dbname, $amount)
    {
        $this->write_to_sock("create database $dbname $amount\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return $res;
    }

    public function drop_database($dbname)
    {
        $this->write_to_sock("drop database $dbname\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return $res;
    }

    public function clear_database($dbname)
    {
        $this->write_to_sock("clear $dbname\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return $res;
    }

    public function resize_database($dbname, $amount)
    {
        $this->write_to_sock("resize $dbname $amount\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return $res;
    }

    public function put($dbname, $name="", array $traits)
    {
        $swrite = "put into $dbname as $name";
        foreach($traits as $trait) {
            $swrite .= " $trait";
        }
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return $res;
    }

    public function update($dbname, $name="", array $traits)
    {
        $swrite = "put into $dbname as $name";
        foreach($traits as $trait) {
            $swrite .= " $trait";
        }
        $this->write_to_sock("$swrite\n");

        $res = $this->read_from_sock();

        if($this->is_error($res)) {
            throw new SDRDBException($res);
        }

        return $res;
    }
}

$client = new SDRDBClient("unix:///tmp/sdrdb.sock");
echo $client->create_database("test2", 100);