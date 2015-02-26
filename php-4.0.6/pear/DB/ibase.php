<?php
//
// +----------------------------------------------------------------------+
// | PHP version 4.0                                                      |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2001 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.02 of the PHP license,      |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Sterling Hughes <sterling@php.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: ibase.php,v 1.18 2001/03/28 00:54:05 cox Exp $
//
// Database independent query interface definition for PHP's Interbase
// extension.
//

require_once 'DB/common.php';

class DB_ibase extends DB_common
{
    var $connection;
    var $phptype, $dbsyntax;
    var $autocommit = 1;
    var $manip_query = array();

    function DB_ibase()
    {
        $this->DB_common();
        $this->phptype = 'ibase';
        $this->dbsyntax = 'ibase';
        $this->features = array(
            'prepare' => true,
            'pconnect' => true,
            'transactions' => true
        );
    }

    function connect($dsninfo, $persistent = false)
    {
        $this->dsn = $dsninfo;
        $user = $dsninfo['username'];
        $pw = $dsninfo['password'];
        $dbhost = $dsninfo['hostspec'] ?
                  ($dsninfo['hostspec'] . ':/' . $dsninfo['database']) :
                  $dsninfo['database'];
        $connect_function = $persistent ? 'ibase_pconnect' : 'ibase_connect';
        if ($dbhost && $user && $pw) {
            $conn = $connect_function($dbhost, $user, $pw);
        } elseif ($dbhost && $user) {
            $conn = $connect_function($dbhost, $user);
        } elseif ($dbhost) {
            $conn = $connect_function($dbhost);
        } else {
            return $this->raiseError("no host, user or password");
        }
        if (!$conn) {
            return $this->raiseError(DB_ERROR_CONNECT_FAILED);
        }
        $this->connection = $conn;
        return DB_OK;
    }

    function disconnect()
    {
        return @ibase_close($this->connection);
    }

    function simpleQuery($query)
    {
        $this->last_query = $query;
        $query = $this->modifyQuery($query);
        $result = @ibase_query($this->connection, $query);
        if (!$result) {
            return $this->raiseError();
        }
        if ($this->autocommit) {
            ibase_commit($this->connection);
        }
        // Determine which queries that should return data, and which
        // should return an error code only.
        return DB::isManip($query) ? DB_OK : $result;
    }

    function &fetchRow($result, $fetchmode=DB_FETCHMODE_DEFAULT)
    {
        if ($fetchmode == DB_FETCHMODE_DEFAULT) {
            $fetchmode = $this->fetchmode;
        }
        if ($fetchmode & DB_FETCHMODE_ASSOC) {
            $row = (array)ibase_fetch_object($result);
        } else {
            $row = ibase_fetch_row($result);
        }
        if (!$row) {
            if ($errmsg = ibase_errmsg()) {
                return $this->raiseError($errmsg);
            } else {
                return null;
            }
        }
        return $row;
    }

    function fetchInto($result, &$ar, $fetchmode=DB_FETCHMODE_DEFAULT, $rownum=null)
    {
        if ($rownum !== NULL) {
            return $this->raiseError(DB_ERROR_NOT_CAPABLE);
        }
        if ($fetchmode == DB_FETCHMODE_DEFAULT) {
            $fetchmode = $this->fetchmode;
        }
        if ($fetchmode & DB_FETCHMODE_ASSOC) {
            return $this->raiseError(DB_ERROR_NOT_CAPABLE);
        } else {
            $ar = ibase_fetch_row($result);
        }
        if (!$ar) {
            if ($errmsg = ibase_errmsg()) {
                return $this->raiseError($errmsg);
            } else {
                return null;
            }
        }
        return DB_OK;
    }

    function freeResult()
    {
        return $this->raiseError(DB_ERROR_NOT_CAPABLE);
    }

    function freeQuery($query)
    {
        ibase_free_query($query);
        return true;
    }

    function numCols($result)
    {
        $cols = ibase_num_fields($result);
        if (!$cols) {
            return $this->raiseError();
        }
        return $cols;
    }

    function prepare($query)
    {
        $this->last_query = $query;
        $query = $this->modifyQuery($query);
        $stmt = ibase_prepare($query);
        $this->manip_query[(int)$stmt] = DB::isManip($query);
        return $stmt;
    }

    function execute($stmt, $data = false)
    {
        $result = ibase_execute($stmt, $data);
        if (!$result) {
            return $this->raiseError();
        }
        if ($this->autocommit) {
            ibase_commit($this->connection);
        }
        return DB::isManip($this->manip_query[(int)$stmt]) ? DB_OK : $result;
    }

    function autoCommit($onoff = false)
    {
        $this->autocommit = $onoff ? 1 : 0;
        return DB_OK;
    }

    function commit()
    {
        return ibase_commit($this->connection);
    }

    function rollback($trans_number)
    {
        return ibase_rollback($this->connection, $trans_number);
    }

    function transactionInit($trans_args = 0)
    {
        return $trans_args ? ibase_trans($trans_args, $this->connection) : ibase_trans();
    }
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */

?>
