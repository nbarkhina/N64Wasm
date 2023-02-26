<?php
# insert base url here (without trailing slash), for example /Home/n64wasm
$baseurl = '';

(!extension_loaded('sqlite3'))?die('sqlite3 extension missing'):'';

function Login() {
	$pw = filter_input(INPUT_GET, 'password');
	$passwordhash = 'BCRYPT_PASSWORD_HASH';
	if (password_verify($pw, $passwordhash)) {
		return true;
	}
	return false;
}

function PrepareDB() {
		print 'preparing database'."\n";
		//create table
		$sql = '
	CREATE TABLE IF NOT EXISTS savestates (
		ID INTEGER PRIMARY KEY AUTOINCREMENT,
		Name TEXT,
		Date TEXT,
		Data BLOB)' ;
		$handle = new SQLite3(__DIR__.'/n64db.sqlite');
		$query = $handle->query($sql);
		if($query){
			print "database prepared\n";
		} else {
			print "error\n";
		}
		$handle->close();
}

function GetTest() {
	return print json_encode(array('message' => 'API Working'));
}


function GetSaveStates(){
	if(Login()){
		$handle = new SQLite3(__DIR__.'/n64db.sqlite');
		$query = $handle->query('select ID,Name,Date from savestates');
		$data = array();
		while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
			$date = preg_replace('/ /', 'T', $row['Date']);
			$data[] = array('ID' => $row['ID'], 'Name' => $row['Name'], 'Date' => $date);
		}
		print json_encode($data);
		$handle->close();
		return;
	}
}

function LoadSaveState(){
	if(Login()){
			$handle = new SQLite3(__DIR__.'/n64db.sqlite');
			$name = urldecode(filter_input(INPUT_GET, 'name'));
			$query = $handle->prepare('SELECT Data FROM savestates WHERE name=? limit 1');
			$query->bindValue(1, $name, SQLITE3_TEXT);
			$result = $query->execute();
			$data = $result->fetchArray(SQLITE3_ASSOC);
			header('Content-Type: application/octet-stream');
			print $data['Data'];
			$handle->close();
			return;
	}
}

function SendSaveState(){
	if(Login()){
		$handle = new SQLite3(__DIR__.'/n64db.sqlite');
		$name = urldecode(filter_input(INPUT_GET, 'name'));
		$query = $handle->prepare('SELECT COUNT(*) FROM savestates WHERE name = ?');
		$query->bindValue(1, $name, SQLITE3_TEXT);
		$result = $query->execute();
		$count = $result->fetchArray(SQLITE3_ASSOC)['COUNT(*)'];
		$data = file_get_contents('php://input');
		if($count == 0){
			$handle->close();
			$handle = new SQLite3(__DIR__.'/n64db.sqlite');
			$query = $handle->prepare("INSERT INTO savestates (Name,Date,Data) VALUES(?,datetime('now'),?)");
			$query->bindValue(1, $name, SQLITE3_TEXT);
			$query->bindValue(2, $data, SQLITE3_BLOB);
		} else {
			$handle->close();
			$handle = new SQLite3(__DIR__.'/n64db.sqlite');
			$query = $handle->prepare("UPDATE savestates SET Data=?,Date=datetime('now') WHERE Name=?");
			$query->bindValue(1, $data, SQLITE3_BLOB);
			$query->bindValue(2, $name, SQLITE3_TEXT);
		}
		$query->execute();
		$handle->close();
		if($query){
			print '"Success"';
		}else{
			print '"Failure"';
		}
	}
}

if(!file_exists(__DIR__.'/n64db.sqlite')){
	PrepareDB();
}
// IIS documentation would have you believe REQUEST_URI would not contain the query string. but this is wrong :(
$request_uri = preg_replace('/\?.+/i', '', $_SERVER['REQUEST_URI']);
if($request_uri == $baseurl.'/api/GetTest'){
	GetTest();
}elseif($request_uri == $baseurl.'/api/Login' && isset($_GET['password'])){
	if(Login()){
		print 'Success'; 
	}else{
		print 'Wrong Password';
	}
}elseif($request_uri == $baseurl.'/api/GetSaveStates' && isset($_GET['password'])){
	header('Content-Type: application/json; charset=utf-8');
	GetSaveStates();
}elseif($request_uri == $baseurl.'/api/SendStaveState' && isset($_GET['password']) && isset($_GET['name'])){
	SendSaveState();
}elseif($request_uri == $baseurl.'/api/LoadStaveState' && isset($_GET['password']) && isset($_GET['name'])){
	LoadSaveState();
}else{
    http_response_code(404);
    print 'Cannot GET '.$_SERVER['REQUEST_URI'];
}
?>