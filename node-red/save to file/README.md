Flow used to store/write temporary data to PC as a file, when the API (arduino.php) can't connect to Database.
When API and Database connection restablished, the flow will read and check if the file contains data or not,
if file contains data, the data will be send again using API and delete the content of the file,
if there is no data in the file, then do nothing.
