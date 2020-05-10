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
            
            if(isset($_POST['delete']))
            {
                deleteFile();
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
        $infotext = "???";
        echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
    }
    
    function printFile()
    {
        $prfile = $_POST['SDfiles'];
        if(strlen($prfile) < 1)
        {
            $infotext = "ERROR: Please SELECT the file to be printed in the list box<br>";
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
        

        // first upload the file
        $target_dir = "phpdir/";
        $base_name = basename($_FILES["fileToUpload"]["name"]);
        $target_file = $target_dir.$base_name;
        $infotext = " ";
        
        switch ($_FILES['fileToUpload']['error']) {
            case UPLOAD_ERR_OK:
                break;
            case UPLOAD_ERR_NO_FILE:
                $infotext = 'No file sent.';
            case UPLOAD_ERR_INI_SIZE:
            case UPLOAD_ERR_FORM_SIZE:
                $infotext = 'Exceeded filesize limit.';
            default:
                $infotext = 'Unknown errors.';
        }   
        
        // and store it in the html folder als "tmp_name"
        if (move_uploaded_file($_FILES["fileToUpload"]["tmp_name"], $target_file)) 
        {
            $infotext = "Upload started: ".basename( $_FILES["fileToUpload"]["name"]);
            header( "refresh:1;url=index.html?reload=".$myrnd);
            echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
            // the file is now located in ...html/phpdir
            // send a message to qidi_connect that the file is available for upload
            sendToqidi_connect($base_name);
        } 
        else 
        {
            $infotext = "ERROR: there was an error uploading your file.";
            header( "refresh:4;url=index.html?reload=".$myrnd);
            echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
        }
    }
    
    function deleteFile()
    {
        $prfile = $_POST['SDfiles'];
        if(strlen($prfile) < 1)
        {
            $infotext = "ERROR: Please SELECT the file to be deleted in the list box<br>";
        }
        else
        {
            $infotext = "Start deleting file :".$prfile."<br>";
            sendToqidi_connect("d".$prfile);
        }
        
        $myrnd = mt_rand();
        header( "refresh:4;url=index.html?reload=".$myrnd);
        echo "<p style='font-size:30px; color:red;'>".$infotext."</p>";
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
