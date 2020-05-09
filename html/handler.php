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
            $infotext = "OK: PRINT command received<br><br>";
            $infotext = $infotext."SELECT the file to print<br>";
        }
        else
        {
            $infotext = "OK: PRINT command received<br><br>";
            $infotext = $infotext."File to Print:".$prfile."<br>";
        }
        
        $myrnd = mt_rand();
        header( "refresh:2;url=index.html?reload=".$myrnd);
        echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
    }
    
    function uploadFile()
    {
    }
    
?>
