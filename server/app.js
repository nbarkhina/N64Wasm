"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const express = require("express");
const sqlite3 = require("sqlite3");
const PASSWORD = "mypassword";
class DBRecord {
}
exports.DBRecord = DBRecord;
class N64Backend {
    constructor() {
    }
    rawBody(req, res, next) {
        //req.setEncoding('utf8');
        if (req.url.includes('/api/SendStaveState')) {
            req.rawBody = '';
            let chunks = [];
            req.on('data', function (chunk) {
                req.rawBody += chunk;
                chunks.push(chunk);
            });
            req.on('end', function () {
                let arrayCount = 0;
                chunks.forEach(chunk => {
                    arrayCount += chunk.length;
                });
                let data = new Uint8Array(arrayCount);
                let counter = 0;
                chunks.forEach(chunk => {
                    for (let i = 0; i < chunk.length; i++) {
                        data[counter] = chunk[i];
                        counter++;
                    }
                });
                req.rawData = data;
                next();
            });
        }
        else {
            //move on to the next middleware
            next();
        }
    }
    runExpress() {
        //sqlite
        this.db = new sqlite3.Database("n64db.sqlite", this.PrepareDB.bind(this));
        this.expressApp = express();
        var thisRef = this;
        // this.expressApp.use(bodyParser.json({ type: 'application/*+json' })); // support json encoded bodies
        // this.expressApp.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies
        // this.expressApp.use(express.json());
        // this.expressApp.use(express.urlencoded()); //Parse URL-encoded bodies
        // this.expressApp.use(express.raw({limit: '5mb',type: null}));
        // this.expressApp.use(bodyParser.raw({limit: '5mb',type: 'application/custom'}))
        // this.expressApp.use(bodyParser.raw({limit: '5mb'}))
        //need to use my own middleware to convert it to a Uint8Array
        this.expressApp.use(this.rawBody);
        this.expressApp.use(express.json());
        this.expressApp.use('/', express.static(__dirname + '/wwwroot', {
            index: 'index.html'
        }));
        this.expressApp.use('/roms', express.static(__dirname + '/roms'));
        this.expressApp.use('/node_modules', express.static(__dirname + '/node_modules'));
        this.expressApp.get('/api/GetTest', function (req, res) {
            thisRef.GetTest(req, res);
        });
        this.expressApp.get('/api/Login', function (req, res) {
            thisRef.Login(req, res);
        });
        this.expressApp.get('/api/GetSaveStates', function (req, res) {
            thisRef.GetSaveStates(req, res);
        });
        this.expressApp.post('/api/SendStaveState', function (req, res) {
            thisRef.SendStaveState(req, res);
        });
        this.expressApp.get('/api/LoadStaveState', function (req, res) {
            thisRef.LoadStaveState(req, res);
        });
        const port = process.env.PORT || 5500;
        this.expressApp.listen(port);
        console.log('server running');
    }
    PrepareDB(err) {
        return __awaiter(this, void 0, void 0, function* () {
            console.log('preparing database');
            //create table
            let sql = `
        CREATE TABLE IF NOT EXISTS savestates (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Name TEXT,
            Date TEXT,
            Data BLOB
            )`;
            yield this.sqlRun(sql);
            //clear table
            //await this.sqlRun(`delete from savestates`);
            console.log('database prepared');
        });
    }
    //need to wrap sqlite oerations in promises because they don't support async/await
    sqlGetRows(sql_statement, params = {}) {
        return new Promise((resolve, reject) => {
            this.db.all(sql_statement, params, (err, rows) => { return (err) ? reject(err) : resolve(rows); });
        });
    }
    sqlRun(sql_statement, params = {}) {
        return new Promise((resolve, reject) => {
            this.db.run(sql_statement, params, (err) => { return (err) ? reject(err) : resolve(null); });
        });
    }
    GetTest(req, res) {
        console.log('Get Test Function');
        res.json({ message: 'API Working' });
    }
    Login(req, res) {
        let pw = req.query.password;
        if (pw != PASSWORD) {
            res.send('Wrong Password');
            return;
        }
        // res.json({ firstname: 'Maddy', lastname: 'Barkhina' });
        res.send('Success');
    }
    GetSaveStates(req, res) {
        return __awaiter(this, void 0, void 0, function* () {
            let pw = req.query.password;
            if (pw != PASSWORD) {
                res.send('Wrong Password');
                return;
            }
            let results = yield this.sqlGetRows(`select ID,Name,Date from savestates`);
            //convert to C Sharp date
            results.forEach(result => {
                result.Date = result.Date.replace(" ", "T");
            });
            // res.send('Success');
            res.json(results);
        });
    }
    LoadStaveState(req, res) {
        return __awaiter(this, void 0, void 0, function* () {
            console.log('load state');
            let pw = req.query.password;
            if (pw != PASSWORD) {
                res.send('Wrong Password');
                return;
            }
            let record = yield this.sqlGetRows(`select Data from savestates where name=$name limit 1`, { $name: req.query.name });
            if (record.length > 0) {
                let result = record[0].Data;
                res.send(result);
            }
            else {
                res.send(null);
            }
        });
    }
    SendStaveState(req, res) {
        return __awaiter(this, void 0, void 0, function* () {
            console.log('save state');
            let pw = req.query.password;
            if (pw != PASSWORD) {
                res.send('Wrong Password');
                return;
            }
            // console.log('length: ' + (req as any).rawBody.length);
            let records = yield this.sqlGetRows(`select ID from savestates where name = $name`, { $name: req.query.name });
            if (records.length == 0) //INSERT
             {
                yield this.sqlRun(`
            insert into savestates (Name,Date,Data)
            values($name,datetime('now'),$data)
            `, {
                    $name: req.query.name,
                    $data: req.rawData
                });
            }
            else //UPDATE
             {
                yield this.sqlRun(`
            update savestates set Data=$data,Date=datetime('now') where Name=$name`, {
                    $name: req.query.name,
                    $data: req.rawData
                });
            }
            // res.json(return_content);
            res.send("\"Success\"");
        });
    }
}
exports.N64Backend = N64Backend;
var monacoBackend = new N64Backend();
monacoBackend.runExpress();
//# sourceMappingURL=app.js.map