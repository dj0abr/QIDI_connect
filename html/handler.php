<?php

    // enter your password her
    // !!! this is HIGHLY recommended if you open this web site to the public !!!
    $my_password = "1234";

    if($_SERVER['REQUEST_METHOD'] == "POST")
    {
        if($_POST['password'] == $my_password)
        {
            if(isset($_POST['print']))
            {
                printFile();
            }
            if(isset($_POST['upload']))
            {
                uploadFile();
            }
        }
        else
        {
            header( "refresh:5;url=index.html" );
            $infotext = "unauthorized access, wrong password";
            echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
        }

    }
    else
    {
        header( "refresh:1;url=index.html");
    }
    
    function printFile()
    {
        $prfile = $_POST['SDfiles'];
        if(strlen($prfile) < 1)
        {
            $infotext = $infotext."ERROR: Please SELECT the file to be printed in the list box<br>";
        }
        else
        {
            $infotext = "Start printig file :".$prfile."<br>";
            sendToqidi_connect($prfile);
        }
        
        $myrnd = mt_rand();
        header( "refresh:4;url=index.html?reload=".$myrnd);
        echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
    }
    
    function uploadFile()
    {
    }
    
    // send a message to the qidi_connect c-program
    function sendToqidi_connect($msg)
    {
        if(!($sock = socket_create(AF_INET, SOCK_DGRAM, 0)))
        {
            echo("SOCKET ERROR ");
        }
        else
        {
            $ret = socket_sendto($sock, $msg, strlen($msg), 0, '127.0.0.1', 8899);
            echo $ret;
            socket_close($sock);
        }
    }
    
?>
