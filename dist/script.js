var AUDIOBUFFSIZE = 1024;

class MyClass {
    constructor() {
        this.rom_name = '';
        this.mobileMode = false;
        this.iosMode = false;
        this.iosVersion = 0;
        this.audioInited = false;
        this.allSaveStates = [];
        this.loginModalOpened = false;
        this.loadSavestateAfterBoot = false;
        this.canvasSize = 640;
        this.eepData = null;
        this.sraData = null;
        this.flaData = null;
        this.dblist = [];
        var Module = {};
        Module['canvas'] = document.getElementById('canvas');
        window['Module'] = Module;
        document.getElementById('file-upload').addEventListener('change', this.uploadRom.bind(this));
        document.getElementById('file-upload-eep').addEventListener('change', this.uploadEep.bind(this));
        document.getElementById('file-upload-sra').addEventListener('change', this.uploadSra.bind(this));
        document.getElementById('file-upload-fla').addEventListener('change', this.uploadFla.bind(this));


        this.rivetsData = {
            message: '',
            beforeEmulatorStarted: true,
            moduleInitializing: true,
            showLogin: false,
            currentFPS: 0,
            audioSkipCount: 0,
            n64SaveStates: [],
            loggedIn: false,
            noCloudSave: true,
            password: '',
            inputController: null,
            remappings: null,
            remapMode: '',
            currKey: 0,
            currJoy: 0,
            chkUseJoypad: false,
            remappingPlayer1: false,
            hasRoms: false,
            romList: [],
            inputLoopStarted: false,
            noLocalSave: true,
            lblError: '',
            chkAdvanced: false,
            eepName: '',
            sraName: '',
            flaName: '',
            swapSticks: false,
            mouseMode: false,
            useZasCMobile: false, //used for starcraft mobile
            showFPS: true,
            invert2P: false,
            invert3P: false,
            invert4P: false,
            disableAudioSync: true,
            forceAngry: false,
            remapPlayer1: true,
            remapOptions: false,
            remapGameshark: false,
            settingMobile: 'Auto',
            iosShowWarning: false,
            cheatName: '',
            cheatAddress: '',
            cheatValue: '',
            cheats: [],
            settings: {
                CLOUDSAVEURL: "",
                SHOWADVANCED: false,
                SHOWOPTIONS: false
            }
        };

        //comes from settings.js
        this.rivetsData.settings = window["N64WASMSETTINGS"];

        if (this.rivetsData.settings.CLOUDSAVEURL!="")
        {
            this.rivetsData.showLogin = true;
        }


        if (window["ROMLIST"].length > 0)
        {
            this.rivetsData.hasRoms = true;
            window["ROMLIST"].forEach(rom => {
                this.rivetsData.romList.push(rom);
            });
        }

        rivets.formatters.ev = function (value, arg) {
            return eval(value + arg);
        }
        rivets.formatters.ev_string = function (value, arg) {
            let eval_string = "'" + value + "'" + arg;
            return eval(eval_string);
        }

        rivets.bind(document.getElementById('topPanel'), { data: this.rivetsData });
        rivets.bind(document.getElementById('bottomPanel'), { data: this.rivetsData });
        rivets.bind(document.getElementById('loginModal'), { data: this.rivetsData });
        rivets.bind(document.getElementById('buttonsModal'), { data: this.rivetsData });
        rivets.bind(document.getElementById('lblError'), { data: this.rivetsData });
        rivets.bind(document.getElementById('mobileBottomPanel'), { data: this.rivetsData });
        rivets.bind(document.getElementById('mobileButtons'), { data: this.rivetsData });
        
        

        this.setupDragDropRom();
        this.detectMobile();
        this.setupLogin();
        this.createDB();
        this.retrieveSettings();

        $('#topPanel').show();
        $('#lblErrorOuter').show();
        
    }

    setupInputController(){
        this.rivetsData.inputController = new InputController();
        

        //try to load keymappings from localstorage
        try {
            let keymappings = localStorage.getItem('n64wasm_mappings_v3');
            if (keymappings) {
                let keymappings_object = JSON.parse(keymappings);

                for (let [key, value] of Object.entries(keymappings_object)) {
                    if (key in this.rivetsData.inputController.KeyMappings){
                        this.rivetsData.inputController.KeyMappings[key] = value;
                    }
                }
            }
        } catch (error) { }
        
    }

    inputLoop(){
        myClass.rivetsData.inputController.update();
        if (myClass.rivetsData.beforeEmulatorStarted)
        {
            setTimeout(() => {
                myClass.inputLoop();
            }, 100);
        }
    }


    processPrintStatement(text) {
        console.log(text);

        //emulator has started event
        if (text.includes('mupen64plus: Starting R4300 emulator: Cached Interpreter')) {
            console.log('detected emulator started');

            if (myClass.loadSavestateAfterBoot)
            {
                setTimeout(() => {
                    myClass.loadCloud();
                }, 500);
            }
            
        }

        //backup sram event
        if (text.includes('writing game.savememory')){
            setTimeout(() => {
                myClass.SaveSram();
            }, 100);
        }
    }

    detectMobile(){
        if (navigator.userAgent.toLocaleLowerCase().includes('iphone'))
        {
            this.iosMode = true;
            try {
                let iosVersion = navigator.userAgent.substring(navigator.userAgent.indexOf("iPhone OS ") + 10);
                iosVersion = iosVersion.substring(0, iosVersion.indexOf(' '));
                iosVersion = iosVersion.substring(0, iosVersion.indexOf('_'));
                this.iosVersion = parseInt(iosVersion);
            } catch (err) { }
    
            if (this.iosVersion > 15) {
                this.rivetsData.iosShowWarning = true;
            }
        }
        if (window.innerWidth < 600 || this.iosMode)
            this.mobileMode = true;
        else
            this.mobileMode = false;
    }

    async LoadEmulator(byteArray){
        if (this.rom_name.toLocaleLowerCase().endsWith('.zip'))
        {
            this.rivetsData.lblError = 'Zip format not supported. Please uncompress first.'
            this.rivetsData.beforeEmulatorStarted = false;
        }
        else
        {
            await this.writeAssets();
            FS.writeFile('custom.v64',byteArray);
            this.beforeRun();
            this.WriteConfigFile();
            this.initAudio(); //need to initAudio before next call for iOS to work
            await this.LoadSram();
            Module.callMain(['custom.v64']);
            this.findInDatabase();
            this.configureEmulator();
            $('#canvasDiv').show();
            this.rivetsData.beforeEmulatorStarted = false;
            this.showToast = Module.cwrap('neil_toast_message', null, ['string']);
            this.toggleFPSModule = Module.cwrap('toggleFPS', null, ['number']);
            this.sendMobileControls = Module.cwrap('neil_send_mobile_controls', null, ['string','string','string']);
            this.setRemainingAudio = Module.cwrap('neil_set_buffer_remaining', null, ['number']);
        }

    }

    async writeAssets(){

        let file = 'assets.zip';
        let responseText = await this.downloadFile(file);
        console.log(file,responseText.length);
        FS.writeFile(
            file, // file name
            responseText
        );
    }

    async downloadFile(url) {
        return new Promise(function (resolve, reject) {
            var oReq = new XMLHttpRequest();
            oReq.open("GET", url, true);
            oReq.responseType = "arraybuffer";
            oReq.onload = function (oEvent) {
                var arrayBuffer = oReq.response;
                var byteArray = new Uint8Array(arrayBuffer);
                resolve(byteArray);
            };
            oReq.onerror = function(){
                reject({
                    status: oReq.status,
                    statusText: oReq.statusText
                });
            }
            oReq.send();
        });
    }

    async initAudio() {

        if (!this.audioInited)
        {
            this.audioInited = true;
            this.audioContext = new AudioContext({
                latencyHint: 'interactive',
                sampleRate: 44100, //this number has to match what's in gui.cpp
            });
            this.gainNode = this.audioContext.createGain();
            this.gainNode.gain.value = 0.5;
            this.gainNode.connect(this.audioContext.destination);
    
            //point at where the emulator is storing the audio buffer
            this.audioBufferResampled = new Int16Array(Module.HEAP16.buffer,Module._neilGetSoundBufferResampledAddress(),64000);
    
            this.audioWritePosition = 0;
            this.audioReadPosition = 0;
            this.audioBackOffCounter = 0;
            this.audioThreadLock = false;
    
    
            //emulator is synced to the OnAudioProcess event because it's way
            //more accurate than emscripten_set_main_loop or RAF
            //and the old method was having constant emulator slowdown swings
            //so the audio suffered as a result
            this.pcmPlayer = this.audioContext.createScriptProcessor(AUDIOBUFFSIZE, 2, 2);
            this.pcmPlayer.onaudioprocess = this.AudioProcessRecurring.bind(this);
            this.pcmPlayer.connect(this.gainNode);
        }

    }

    hasEnoughSamples(){

        let readPositionTemp = this.audioReadPosition;
        let enoughSamples = true;
        for (let sample = 0; sample < AUDIOBUFFSIZE; sample++)
        {
            if (this.audioWritePosition != readPositionTemp) {
                readPositionTemp += 2;

                //wrap back around within the ring buffer
                if (readPositionTemp == 64000) {
                    readPositionTemp = 0;
                }
            }
            else {
                enoughSamples = false;
            }
        }

        return enoughSamples;
    }

    //this method keeps getting called when it needs more audio
    //data to play so we just keep streaming it from the emulator
    AudioProcessRecurring(audioProcessingEvent){

        //I think this method is thread safe but just in case
        if (this.audioThreadLock || this.rivetsData.beforeEmulatorStarted)
        {
            // console.log('audio thread dupe');
            return;
        }
        
        this.audioThreadLock = true;



        var sampleRate = audioProcessingEvent.outputBuffer.sampleRate;
        let outputBuffer = audioProcessingEvent.outputBuffer;
        let outputData1 = outputBuffer.getChannelData(0);
        let outputData2 = outputBuffer.getChannelData(1);

        if (this.rivetsData.disableAudioSync)
        {
            this.audioWritePosition = Module._neilGetAudioWritePosition();
        }
        else
        {
            Module._runMainLoop();

            this.audioWritePosition = Module._neilGetAudioWritePosition();
    
    
            if (!this.hasEnoughSamples())
            {
                Module._runMainLoop();
            }
    
            this.audioWritePosition = Module._neilGetAudioWritePosition();
        }

    

        // if (!this.hasEnoughSamples())
        //     console.log('not enough samples');

        // console.log('Write: ' + this.audioWritePosition + ' Read: ' + this.audioReadPosition);

        let hadSkip = false;


        //the bytes are arranged L,R,L,R,etc.... for each speaker
        for (let sample = 0; sample < AUDIOBUFFSIZE; sample++) {

            if (this.audioWritePosition != this.audioReadPosition) {
                outputData1[sample] = (this.audioBufferResampled[this.audioReadPosition] / 32768);
                outputData2[sample] = (this.audioBufferResampled[this.audioReadPosition + 1] / 32768);

                this.audioReadPosition += 2;

                //wrap back around within the ring buffer
                if (this.audioReadPosition == 64000) {
                    this.audioReadPosition = 0;
                }
            }
            else {
                //if there's nothing to play then just play silence
                outputData1[sample] = 0;
                outputData2[sample] = 0;

                //if we caught up on samples then back off
                //for 2 frames to buffer some audio
                // if (this.audioBackOffCounter == 0) {
                //     this.audioBackOffCounter = 2;
                // }

                hadSkip = true;

            }

        }

        
        if (hadSkip)
            this.rivetsData.audioSkipCount++;

        //calculate remaining audio in buffer
        let audioBufferRemaining = 0;
        let readPositionTemp = this.audioReadPosition;
        let writePositionTemp = this.audioWritePosition;
        for(let i = 0; i < 64000; i++)
        {
            if (readPositionTemp != writePositionTemp)
            {
                readPositionTemp += 2;
                audioBufferRemaining += 2;

                if (readPositionTemp == 64000) {
                    readPositionTemp = 0;
                }
            }
        }

        this.setRemainingAudio(audioBufferRemaining);
        //myClass.showToast("Buffer: " + audioBufferRemaining);
        
        this.audioThreadLock = false;

    }

    beforeRun(){
        //add any overriding logic here before the emulator starts
    }

    WriteConfigFile()
    {
        let configString = "";

        //gamepad
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Up + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Down + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Left + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Right + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_A + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_B + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_Start + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_Z + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_L + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_R + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Menu + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_CLEFT + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_CRIGHT + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_CUP + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Joy_Mapping_Action_CDOWN + "\r\n";

        //keyboard
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Left + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Right + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Up + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Down + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_Start + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_CUP + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_CDOWN + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_CLEFT + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_CRIGHT + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_Z + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_L + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_R + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_B + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_A + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Menu + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_Analog_Up + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_Analog_Down + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_Analog_Left + "\r\n";
        configString += this.rivetsData.inputController.KeyMappings.Mapping_Action_Analog_Right + "\r\n";

        //load save files
        if (this.eepData == null) configString += "0" + "\r\n"; else configString += "1" + "\r\n";
        if (this.sraData == null) configString += "0" + "\r\n"; else configString += "1" + "\r\n";
        if (this.flaData == null) configString += "0" + "\r\n"; else configString += "1" + "\r\n";

        //show FPS
        if (this.rivetsData.showFPS) configString += "1" + "\r\n"; else configString += "0" + "\r\n";

        //swap sticks
        if (this.rivetsData.swapSticks) configString += "1" + "\r\n"; else configString += "0" + "\r\n";

        //disable audio sync
        if (this.rivetsData.disableAudioSync) configString += "1" + "\r\n"; else configString += "0" + "\r\n";

        //invert player Y axis
        if (this.rivetsData.invert2P) configString += "1" + "\r\n"; else configString += "0" + "\r\n";
        if (this.rivetsData.invert3P) configString += "1" + "\r\n"; else configString += "0" + "\r\n";
        if (this.rivetsData.invert4P) configString += "1" + "\r\n"; else configString += "0" + "\r\n";

        //mobile mode
        if (this.rivetsData.settingMobile == 'ForceMobile') this.mobileMode = true;
        if (this.rivetsData.settingMobile == 'ForceDesktop') this.mobileMode = false;
        if (this.mobileMode) configString += "1" + "\r\n"; else configString += "0" + "\r\n";
    
        //angrylion software renderer
        if (this.rivetsData.forceAngry) configString += "1" + "\r\n"; else configString += "0" + "\r\n";

        //mouse mode
        if (this.rivetsData.mouseMode) configString += "1" + "\r\n"; else configString += "0" + "\r\n";

        FS.writeFile('config.txt',configString);

        //write cheats
        let cheatString = '';
        this.rivetsData.cheats.forEach(cheat => {
            if (cheat.active)
            {
                cheatString += cheat.address + "\r\n" + cheat.value + "\r\n";
            }
        });
        
        FS.writeFile('cheat.txt',cheatString);

    }


    uploadBrowse() {
        this.initAudio();
        document.getElementById('file-upload').click();
    }

    uploadEepBrowse() {
        document.getElementById('file-upload-eep').click();
    }
    uploadSraBrowse() {
        document.getElementById('file-upload-sra').click();
    }
    uploadFlaBrowse() {
        document.getElementById('file-upload-fla').click();
    }

    uploadEep(event) {
        var file = event.currentTarget.files[0];
        console.log(file);
        myClass.rivetsData.eepName = 'File Ready';
        var reader = new FileReader();
        reader.onprogress = function (e) {
            console.log('loaded: ' + e.loaded);
        };
        reader.onload = function (e) {
            console.log('finished loading');
            var byteArray = new Uint8Array(this.result);
            myClass.eepData = byteArray;

            FS.writeFile(
                "game.eep", // file name
                byteArray
            );
        }
        reader.readAsArrayBuffer(file);
    }
    uploadSra(event) {
        var file = event.currentTarget.files[0];
        console.log(file);
        myClass.rivetsData.sraName = 'File Ready';
        var reader = new FileReader();
        reader.onprogress = function (e) {
            console.log('loaded: ' + e.loaded);
        };
        reader.onload = function (e) {
            console.log('finished loading');
            var byteArray = new Uint8Array(this.result);
            myClass.sraData = byteArray;

            FS.writeFile(
                "game.sra", // file name
                byteArray
            );
        }
        reader.readAsArrayBuffer(file);
    }
    uploadFla(event) {
        var file = event.currentTarget.files[0];
        console.log(file);
        myClass.rivetsData.flaName = 'File Ready';
        var reader = new FileReader();
        reader.onprogress = function (e) {
            console.log('loaded: ' + e.loaded);
        };
        reader.onload = function (e) {
            console.log('finished loading');
            var byteArray = new Uint8Array(this.result);
            myClass.flaData = byteArray;

            FS.writeFile(
                "game.fla", // file name
                byteArray
            );
        }
        reader.readAsArrayBuffer(file);
    }

    uploadRom(event) {
        var file = event.currentTarget.files[0];
        myClass.rom_name = file.name;
        console.log(file);
        var reader = new FileReader();
        reader.onprogress = function (e) {
            console.log('loaded: ' + e.loaded);
        };
        reader.onload = function (e) {
            console.log('finished loading');
            var byteArray = new Uint8Array(this.result);
            myClass.LoadEmulator(byteArray);
        }
        reader.readAsArrayBuffer(file);
    }

    resizeCanvas() {
        $('#canvas').width(this.canvasSize);
    }

    zoomOut() {

        this.canvasSize -= 50;
        localStorage.setItem('n64wasm-size', this.canvasSize.toString());
        this.resizeCanvas();
    }

    zoomIn() {
        this.canvasSize += 50;
        localStorage.setItem('n64wasm-size', this.canvasSize.toString());
        this.resizeCanvas();
    }


    async initModule(){
        console.log('module initialized');
        myClass.rivetsData.moduleInitializing = false;
        //myClass.loadFiles();
    }

    //not being used currently
    async loadFiles(){
        let files = ["shader_vert.hlsl", "shader_frag.hlsl"];

        for (let i = 0; i < files.length; i++) {
            let file = files[i];
            let responseText = await $.ajax({
                url: file,
                beforeSend: function (xhr) {
                    xhr.overrideMimeType("text/plain; charset=x-user-defined");
                }
            });
            console.log(file,responseText.length);
            FS.writeFile(
                file, // file name
                responseText
            );
        }
    }

    //DRAG AND DROP ROM
    setupDragDropRom(){
        let dropArea = document.getElementById('dropArea');

        dropArea.addEventListener('dragenter', this.preventDefaults, false);
        dropArea.addEventListener('dragover', this.preventDefaults, false);
        dropArea.addEventListener('dragleave', this.preventDefaults, false);
        dropArea.addEventListener('drop', this.preventDefaults, false);
        
        dropArea.addEventListener('dragenter', this.dragDropHighlight, false);
        dropArea.addEventListener('dragover', this.dragDropHighlight, false);
        dropArea.addEventListener('dragleave', this.dragDropUnHighlight, false);
        dropArea.addEventListener('drop', this.dragDropUnHighlight, false);

        dropArea.addEventListener('drop', this.handleDrop, false);

    }

    preventDefaults(e){
        e.preventDefault();
        e.stopPropagation();
    }

    dragDropHighlight(e){
        $('#dropArea').css({"background-color": "lightblue"});
    }

    dragDropUnHighlight(e){
        $('#dropArea').css({"background-color": "inherit"});
    }

    handleDrop(e){
        let dt = e.dataTransfer;
        let files = dt.files;

        var file = files[0];
        myClass.rom_name = file.name;
        console.log(file);
        var reader = new FileReader();
        reader.onprogress = function (e) {
            console.log('loaded: ' + e.loaded);
        };
        reader.onload = function (e) {
            console.log('finished loading');
            var byteArray = new Uint8Array(this.result);
            myClass.LoadEmulator(byteArray);
        }
        reader.readAsArrayBuffer(file);

    }

    extractRomName(name){
        if (name.includes('/'))
        {
            name = name.substr(name.lastIndexOf('/')+1);
        }

        return name;
    }

    loadRomAndSavestate(){
        let selector = document.getElementById('romselect');
        let saveToLoad = document.getElementById('savestateSelect')["value"];

        for (let i=0;i<selector.options.length;i++)
        {
            let romurl = selector.options[i].value;
            let romname = this.extractRomName(romurl);

            if (saveToLoad==romname + '.n64wasm')
            {
                selector.selectedIndex = i; 
                this.loadSavestateAfterBoot = true;
            }
        }

        this.loadRom();
    }

    async loadRom() {
        //get rom url
        let romurl = document.getElementById('romselect')["value"];
        console.log(romurl);
        this.rom_name = this.extractRomName(romurl);

        this.load_url(romurl);
    }

    load_url(path) {
        console.log('loading ' + path);

        var req = new XMLHttpRequest();
        req.open("GET", path);
        req.overrideMimeType("text/plain; charset=x-user-defined");
        req.onerror = () => console.log(`Error loading ${path}: ${req.statusText}`);
        req.responseType = "arraybuffer";

        req.onload = function () {
            var arrayBuffer = req.response; // Note: not oReq.responseText
            try{
                if (arrayBuffer) {
                    var byteArray = new Uint8Array(arrayBuffer);
                    myClass.LoadEmulator(byteArray);
                }
                else{
                    //toastr.error('Error Loading Cloud Save');
                }
            }
            catch(error){
                console.log(error);
                toastr.error('Error Loading Save');
            }
        };

        req.send();
    }

    

    saveStateLocal(){
        console.log('saveStateLocal');
        this.rivetsData.noLocalSave = false;
        Module._neil_serialize();
    }

    loadStateLocal(){
        console.log('loadStateLocal');
        myClass.loadFromDatabase();
    }

    createDB() {

        if (window["indexedDB"]==undefined){
            console.log('indexedDB not available');
            return;
        }

        var request = indexedDB.open('N64WASMDB');
        request.onupgradeneeded = function (ev) {
            console.log('upgrade needed');
            let db = ev.target.result;
            let objectStore = db.createObjectStore('N64WASMSTATES', { autoIncrement: true });
            objectStore.transaction.oncomplete = function (event) {
                console.log('db created');
            };
        }

        request.onsuccess = function (ev) {
            var db = ev.target.result;
            var romStore = db.transaction("N64WASMSTATES", "readwrite").objectStore("N64WASMSTATES");
            try {
                //rewrote using cursor instead of getAllKeys
                //for compatibility with MS EDGE
                romStore.openCursor().onsuccess = function (ev) {
                    var cursor = ev.target.result;
                    if (cursor) {
                        let rom = cursor.key.toString();
                        myClass.dblist.push(rom);
                        cursor.continue();
                    }
                    else {
                        if (myClass.dblist.length > 0) {
                            //TODO show savestates grid
                        }
                    }
                }

            } catch (error) {
                console.log('error reading keys');
                console.log(error);
            }

        }

    }

    findInDatabase() {

        if (!window["indexedDB"]==undefined){
            console.log('indexedDB not available');
            return;
        }
        
        var request = indexedDB.open('N64WASMDB');
        request.onsuccess = function (ev) {
            var db = ev.target.result;
            var romStore = db.transaction("N64WASMSTATES", "readwrite").objectStore("N64WASMSTATES");
            try {
                romStore.openCursor().onsuccess = function (ev) {
                    var cursor = ev.target.result;
                    if (cursor) {
                        let rom = cursor.key.toString();
                        if (myClass.rom_name == rom)
                        {
                            myClass.rivetsData.noLocalSave = false;
                        }
                        cursor.continue();
                    }
                }

            } catch (error) {
                console.log('error reading keys');
                console.log(error);
            }
        }
    }

    async LoadSram() {

        return new Promise(function (resolve, reject) {
            var request = indexedDB.open('N64WASMDB');
            try
            {
                request.onsuccess = function (ev) {
                    var db = ev.target.result;
                    var romStore = db.transaction("N64WASMSTATES", "readwrite").objectStore("N64WASMSTATES");
                    var rom = romStore.get(myClass.rom_name + '.sram');
                    rom.onsuccess = function (event) {
                        if (rom.result)
                        {
                            let byteArray = rom.result; //Uint8Array
                            FS.writeFile('/game.savememory', byteArray);
                        }
                        resolve();
                    };
                    rom.onerror = function (event) {
                        reject();
                    }
                }
                request.onerror = function (ev) {
                    reject();
                }
            }catch(error){
                reject();
            }
        });

    }

    SaveSram() {

        let data = FS.readFile('/game.savememory'); //this is a Uint8Array

        var request = indexedDB.open('N64WASMDB');
        request.onsuccess = function (ev) {
            var db = ev.target.result;
            var romStore = db.transaction("N64WASMSTATES", "readwrite").objectStore("N64WASMSTATES");
            var addRequest = romStore.put(data, myClass.rom_name + '.sram');
            addRequest.onsuccess = function (event) {
                console.log('sram added');
            };
            addRequest.onerror = function (event) {
                console.log('error adding sram');
                console.log(event);
            };
        }

    }

    saveToDatabase(data) {

        if (!window["indexedDB"]==undefined){
            console.log('indexedDB not available');
            return;
        }
        
        console.log('save to database called: ', data.length);

        var request = indexedDB.open('N64WASMDB');
        request.onsuccess = function (ev) {
            var db = ev.target.result;
            var romStore = db.transaction("N64WASMSTATES", "readwrite").objectStore("N64WASMSTATES");
            var addRequest = romStore.put(data, myClass.rom_name);
            addRequest.onsuccess = function (event) {
                console.log('data added');
                toastr.info('State Saved');
                window["myApp"].showToast("State Saved");
            };
            addRequest.onerror = function (event) {
                console.log('error adding data');
                console.log(event);
            };
        }
    }


    loadFromDatabase() {

        var request = indexedDB.open('N64WASMDB');
        request.onsuccess = function (ev) {
            var db = ev.target.result;
            var romStore = db.transaction("N64WASMSTATES", "readwrite").objectStore("N64WASMSTATES");
            var rom = romStore.get(myClass.rom_name);
            rom.onsuccess = function (event) {
                let byteArray = rom.result; //Uint8Array
                FS.writeFile('/savestate.gz',byteArray);
                Module._neil_unserialize();

            };
            rom.onerror = function (event) {
                toastr.error('error getting rom from store');
            }
        }
        request.onerror = function (ev) {
            toastr.error('error loading from db')
        }

    }


    clearDatabase() {

        var request = indexedDB.deleteDatabase('N64WASMDB');
        request.onerror = function (event) {
            console.log("Error deleting database.");
            toastr.error("Error deleting database");
        };

        request.onsuccess = function (event) {
            console.log("Database deleted successfully");
            toastr.error("Database deleted successfully");
        };

    }
    

    exportEep(){
        Module._neil_export_eep();
    }
    ExportEepEvent()
    {
        console.log('js eep event');

        let filearray = FS.readFile("/game.eep");   
        var file = new File([filearray], "game.eep", {type: "text/plain; charset=x-user-defined"});
        saveAs(file);
    }
    exportSra(){
        Module._neil_export_sra();
    }
    ExportSraEvent()
    {
        console.log('js sra event');

        let filearray = FS.readFile("/game.sra");   
        var file = new File([filearray], "game.sra", {type: "text/plain; charset=x-user-defined"});
        saveAs(file);
    }
    exportFla(){
        Module._neil_export_fla();
    }
    ExportFlaEvent()
    {
        console.log('js fla event');

        let filearray = FS.readFile("/game.fla");   
        var file = new File([filearray], "game.fla", {type: "text/plain; charset=x-user-defined"});
        saveAs(file);
    }

    //when it returns from emscripten
    SaveStateEvent()
    {
        myClass.hideMobileMenu();

        console.log('js savestate event');
        let compressed = FS.readFile('/savestate.gz'); //this is a Uint8Array

        //use local db
        if (!myClass.rivetsData.loggedIn)
        {
            myClass.saveToDatabase(compressed);
            return;
        }


        var xhr = new XMLHttpRequest;
        xhr.open("POST", this.rivetsData.settings.CLOUDSAVEURL + "/SendStaveState?name=" + this.rom_name + '.n64wasm' + 
            "&password=" + this.rivetsData.password + "&emulator=n64", true);
        xhr.send(compressed);

        xhr.onreadystatechange = function() {
            try{
                if (xhr.readyState === 4) {
                    let result = xhr.response;
                    if (result=="\"Success\""){
                        myClass.rivetsData.noCloudSave = false;
                        toastr.info("Cloud State Saved");
                        myClass.showToast("Cloud State Saved");
                    }else{
                        toastr.error('Error Saving Cloud Save');
                    }
                }
            }
            catch(error){
                console.log(error);
                toastr.error('Error Loading Cloud Save');
            }
            
        }
    }

    saveCloud(){
        Module._neil_serialize();
    }

    loadCloud(){

        myClass.hideMobileMenu();

        //use local db
        if (!myClass.rivetsData.loggedIn)
        {
            myClass.loadFromDatabase();
            return;
        }

        var oReq = new XMLHttpRequest();
        oReq.open("GET", this.rivetsData.settings.CLOUDSAVEURL + "/LoadStaveState?name=" + this.rom_name + '.n64wasm' +
         "&password=" + this.rivetsData.password, true);
        oReq.responseType = "arraybuffer";

        oReq.onload = function (oEvent) {
            var arrayBuffer = oReq.response; // Note: not oReq.responseText
            try{
                if (arrayBuffer) {
                    var byteArray = new Uint8Array(arrayBuffer);
                    FS.writeFile('/savestate.gz',byteArray);
                    Module._neil_unserialize();
                }
                else{
                    toastr.error('Error Loading Cloud Save');
                }
            }
            catch(error){
                console.log(error);
                toastr.error('Error Loading Cloud Save');
            }
            
        };

        oReq.send(null);
    }

    fullscreen() {
        try {
            let el = document.getElementById('canvas');

            if (el.webkitRequestFullScreen) {
                el.webkitRequestFullScreen();
            }
            else {
                el.mozRequestFullScreen();
            }
        } catch (error) 
        { 
            console.log('full screen failed');
        }
    }

    newRom(){
        location.reload();
    }

    configureEmulator(){
        let size = localStorage.getItem('n64wasm-size');
        if (size) {
            console.log('size found');
            let sizeNum = parseInt(size);
            this.canvasSize = sizeNum;
        }

        if (this.mobileMode)
        {
            this.setupMobileMode();
        }

        this.resizeCanvas();

        if (this.rivetsData.password)
            this.loginSilent();

        if (this.rivetsData.mouseMode)
        {
            document.getElementById('canvasDiv').addEventListener("click", this.canvasClick.bind(this));
        }
    }

    canvasClick(){
        let isPointerCurrentlyLocked = document.pointerLockElement;
        if (!isPointerCurrentlyLocked)
            this.captureMouse();
    }

    captureMouse(){
        let canvas = document.getElementById('canvas');

        //mouse capture
        canvas.requestPointerLock = canvas.requestPointerLock ||
        canvas.mozRequestPointerLock;

        canvas.requestPointerLock()
    }

    setupMobileMode() {

        this.canvasSize = window.innerWidth;
        console.log('canvas size', this.canvasSize);

        $("#btnHideMenu").show();
        let halfWidth = (window.innerWidth / 2) - 35;
        document.getElementById("menuDiv").style.left = halfWidth + "px";

        this.rivetsData.inputController.setupMobileControls('divTouchSurface');

        // document.getElementById('body').style['background-color'] = 'white';

        $("#mobileDiv").show();
        $("#maindiv").hide();
        $('#canvas').appendTo("#mobileCanvas");

        document.getElementById('maindiv').classList.remove('container');

        //fixes the small gap between canvas and mobile buttons
        document.getElementById('canvas').style.display = 'block';

        //scroll back to top
        try {
            document.body.scrollTop = 0; // For Safari
            document.documentElement.scrollTop = 0; // For Chrome, Firefox, IE and Opera
        } catch (error) { }
    }

    hideMobileMenu() {
        if (this.mobileMode)
        {
            $("#mobileButtons").hide();
            $('#menuDiv').show();
        }
    }

    setFromLocalStorage(localStorageName, rivetsName){
        if (localStorage.getItem(localStorageName))
        {
            if (localStorage.getItem(localStorageName)=="true")
                this.rivetsData[rivetsName] = true;
            else if (localStorage.getItem(localStorageName)=="false")
                this.rivetsData[rivetsName] = false;
            else
                this.rivetsData[rivetsName] = localStorage.getItem(localStorageName);
        }
    }

    setToLocalStorage(localStorageName, rivetsName){

        if (typeof(myApp.rivetsData[rivetsName]) == 'boolean')
        {
            if (this.rivetsData[rivetsName])
                localStorage.setItem(localStorageName, 'true');
            else        
                localStorage.setItem(localStorageName, 'false');
        }
        else
        {
            localStorage.setItem(localStorageName, this.rivetsData[rivetsName]);
        }

    }

    retrieveSettings(){
        this.loadCheats();
        this.setFromLocalStorage('n64wasm-showfps','showFPS');
        this.setFromLocalStorage('n64wasm-disableaudiosyncnew','disableAudioSync');
        this.setFromLocalStorage('n64wasm-swapSticks','swapSticks');
        this.setFromLocalStorage('n64wasm-invert2P','invert2P');
        this.setFromLocalStorage('n64wasm-invert3P','invert3P');
        this.setFromLocalStorage('n64wasm-invert4P','invert4P');
        this.setFromLocalStorage('n64wasm-settingMobile','settingMobile');
        this.setFromLocalStorage('n64wasm-mouseMode','mouseMode');
        this.setFromLocalStorage('n64wasm-forceAngry','forceAngry');

    }

    saveOptions(){

        this.rivetsData.showFPS = this.rivetsData.showFPSTemp;
        this.rivetsData.swapSticks = this.rivetsData.swapSticksTemp;
        this.rivetsData.mouseMode = this.rivetsData.mouseModeTemp;
        this.rivetsData.invert2P = this.rivetsData.invert2PTemp;
        this.rivetsData.invert3P = this.rivetsData.invert3PTemp;
        this.rivetsData.invert4P = this.rivetsData.invert4PTemp;
        this.rivetsData.disableAudioSync = this.rivetsData.disableAudioSyncTemp;
        this.rivetsData.settingMobile = this.rivetsData.settingMobileTemp;
        this.rivetsData.forceAngry = this.rivetsData.forceAngryTemp;

        this.setToLocalStorage('n64wasm-showfps','showFPS');
        this.setToLocalStorage('n64wasm-disableaudiosyncnew','disableAudioSync');
        this.setToLocalStorage('n64wasm-swapSticks','swapSticks');
        this.setToLocalStorage('n64wasm-mouseMode','mouseMode');
        this.setToLocalStorage('n64wasm-invert2P','invert2P');
        this.setToLocalStorage('n64wasm-invert3P','invert3P');
        this.setToLocalStorage('n64wasm-invert4P','invert4P');
        this.setToLocalStorage('n64wasm-settingMobile','settingMobile');
        this.setToLocalStorage('n64wasm-forceAngry','forceAngry');
        
    }


    showRemapModal() {

        this.rivetsData.remapPlayer1 = true;
        this.rivetsData.remapOptions = false;
        this.rivetsData.remapGameshark = false;

        this.rivetsData.showFPSTemp = this.rivetsData.showFPS;
        this.rivetsData.swapSticksTemp = this.rivetsData.swapSticks;
        this.rivetsData.mouseModeTemp = this.rivetsData.mouseMode;
        this.rivetsData.invert2PTemp = this.rivetsData.invert2P;
        this.rivetsData.invert3PTemp = this.rivetsData.invert3P;
        this.rivetsData.invert4PTemp = this.rivetsData.invert4P;
        this.rivetsData.disableAudioSyncTemp = this.rivetsData.disableAudioSync;
        this.rivetsData.settingMobileTemp = this.rivetsData.settingMobile;
        this.rivetsData.forceAngryTemp = this.rivetsData.forceAngry;

        //start input loop
        if (!this.rivetsData.inputLoopStarted)
        {
            this.rivetsData.inputLoopStarted = true;
            this.rivetsData.inputController.setupGamePad();
            setTimeout(() => {
                myClass.inputLoop();
            }, 100);
        }
        
        if (this.rivetsData.inputController.Gamepad_Process_Axis)
            this.rivetsData.chkUseJoypad = true;
        this.rivetsData.remappings = JSON.parse(JSON.stringify(this.rivetsData.inputController.KeyMappings));
        this.rivetsData.remapWait = false;
        $("#buttonsModal").modal();
    }

    addCheat(){
        let cheat = {
            name: this.rivetsData.cheatName.trim(),
            address: this.rivetsData.cheatAddress.trim(),
            value: this.rivetsData.cheatValue.trim(),
            active: true
        }

        this.rivetsData.cheats.push(cheat);
        this.rivetsData.cheatName = '';
        this.rivetsData.cheatAddress = '';
        this.rivetsData.cheatValue = '';

        this.saveCheats();
    }

    loadCheats(){
        try
        {
            let cheats = JSON.parse(localStorage.getItem('n64wasm-cheats'));
            for(let i = 0; i < cheats.length; i++)
            {
                let cheat = cheats[i];
                if (cheat.name && cheat.address && cheat.value)
                {
                    this.rivetsData.cheats.push(cheat);
                }
            }
        }catch(err){}
    }

    saveCheats(){
        localStorage.setItem('n64wasm-cheats',JSON.stringify(this.rivetsData.cheats));
    }

    deleteCheat(cheat){
        this.rivetsData.cheats = this.rivetsData.cheats.filter((a)=>{ return a.name != cheat.name; });
        this.saveCheats();
    }

    swapRemap(id){
        if (id=='options')
        {
            this.rivetsData.remapPlayer1 = false;
            this.rivetsData.remapOptions = true;
            this.rivetsData.remapGameshark = false;
        }
        if (id=='player1')
        {
            this.rivetsData.remapPlayer1 = true;
            this.rivetsData.remapOptions = false;
            this.rivetsData.remapGameshark = false;
        }
        if (id=='gameshark')
        {
            this.rivetsData.remapPlayer1 = false;
            this.rivetsData.remapOptions = false;
            this.rivetsData.remapGameshark = true;
        }
    }
    

    saveRemap() {
        if (this.rivetsData.chkUseJoypad)
            this.rivetsData.inputController.Gamepad_Process_Axis = true;
        else
            this.rivetsData.inputController.Gamepad_Process_Axis = false;

        this.rivetsData.inputController.KeyMappings = JSON.parse(JSON.stringify(this.rivetsData.remappings));
        this.rivetsData.inputController.setGamePadButtons();
        this.saveOptions();
        localStorage.setItem('n64wasm_mappings_v3', JSON.stringify(this.rivetsData.remappings));
        $("#buttonsModal").modal('hide');
    }

    btnRemapKey(keynum) {
        console.log(this);
        this.rivetsData.currKey = keynum;
        this.rivetsData.remapMode = 'Key';
        this.readyRemap();
    }

    btnRemapJoy(joynum) {

        this.rivetsData.currJoy = joynum;
        this.rivetsData.remapMode = 'Button';
        this.readyRemap();
    }

    readyRemap() {
        this.rivetsData.remapWait = true;
        this.rivetsData.inputController.Key_Last = '';
        this.rivetsData.inputController.Joy_Last = null;
        this.rivetsData.inputController.Remap_Check = true;
    }

    restoreDefaultKeymappings(){
        this.rivetsData.remappings = this.rivetsData.inputController.defaultKeymappings();
        this.rivetsData.showFPSTemp = true;
        this.rivetsData.swapSticksTemp = false;
        this.rivetsData.mouseModeTemp = false;
        this.rivetsData.invert2PTemp = false;
        this.rivetsData.invert3PTemp = false;
        this.rivetsData.invert4PTemp = false;
        this.rivetsData.disableAudioSyncTemp = true;
        this.rivetsData.settingMobileTemp = 'Auto';
        this.rivetsData.forceAngryTemp = false;
    }

    remapPressed() {
        if (this.rivetsData.remapMode == 'Key') {
            var keyLast = this.rivetsData.inputController.Key_Last;

            //player 1
            if (this.rivetsData.currKey == 1) this.rivetsData.remappings.Mapping_Up = keyLast;
            if (this.rivetsData.currKey == 2) this.rivetsData.remappings.Mapping_Down = keyLast;
            if (this.rivetsData.currKey == 3) this.rivetsData.remappings.Mapping_Left = keyLast;
            if (this.rivetsData.currKey == 4) this.rivetsData.remappings.Mapping_Right = keyLast;
            if (this.rivetsData.currKey == 5) this.rivetsData.remappings.Mapping_Action_A = keyLast;
            if (this.rivetsData.currKey == 6) this.rivetsData.remappings.Mapping_Action_B = keyLast;
            if (this.rivetsData.currKey == 8) this.rivetsData.remappings.Mapping_Action_Start = keyLast;
            if (this.rivetsData.currKey == 9) this.rivetsData.remappings.Mapping_Menu = keyLast;
            if (this.rivetsData.currKey == 10) this.rivetsData.remappings.Mapping_Action_Z = keyLast;
            if (this.rivetsData.currKey == 11) this.rivetsData.remappings.Mapping_Action_L = keyLast;
            if (this.rivetsData.currKey == 12) this.rivetsData.remappings.Mapping_Action_R = keyLast;
            if (this.rivetsData.currKey == 13) this.rivetsData.remappings.Mapping_Action_CUP = keyLast;
            if (this.rivetsData.currKey == 14) this.rivetsData.remappings.Mapping_Action_CDOWN = keyLast;
            if (this.rivetsData.currKey == 15) this.rivetsData.remappings.Mapping_Action_CLEFT = keyLast;
            if (this.rivetsData.currKey == 16) this.rivetsData.remappings.Mapping_Action_CRIGHT = keyLast;
            if (this.rivetsData.currKey == 17) this.rivetsData.remappings.Mapping_Action_Analog_Up = keyLast;
            if (this.rivetsData.currKey == 18) this.rivetsData.remappings.Mapping_Action_Analog_Down = keyLast;
            if (this.rivetsData.currKey == 19) this.rivetsData.remappings.Mapping_Action_Analog_Left = keyLast;
            if (this.rivetsData.currKey == 20) this.rivetsData.remappings.Mapping_Action_Analog_Right = keyLast;

        }
        if (this.rivetsData.remapMode == 'Button') {
            var joyLast = this.rivetsData.inputController.Joy_Last;
            if (this.rivetsData.currJoy == 1) this.rivetsData.remappings.Joy_Mapping_Up = joyLast;
            if (this.rivetsData.currJoy == 2) this.rivetsData.remappings.Joy_Mapping_Down = joyLast;
            if (this.rivetsData.currJoy == 3) this.rivetsData.remappings.Joy_Mapping_Left = joyLast;
            if (this.rivetsData.currJoy == 4) this.rivetsData.remappings.Joy_Mapping_Right = joyLast;
            if (this.rivetsData.currJoy == 5) this.rivetsData.remappings.Joy_Mapping_Action_A = joyLast;
            if (this.rivetsData.currJoy == 6) this.rivetsData.remappings.Joy_Mapping_Action_B = joyLast;
            if (this.rivetsData.currJoy == 8) this.rivetsData.remappings.Joy_Mapping_Action_Start = joyLast;
            if (this.rivetsData.currJoy == 9) this.rivetsData.remappings.Joy_Mapping_Menu = joyLast;
            if (this.rivetsData.currJoy == 10) this.rivetsData.remappings.Joy_Mapping_Action_Z = joyLast;
            if (this.rivetsData.currJoy == 11) this.rivetsData.remappings.Joy_Mapping_Action_L = joyLast;
            if (this.rivetsData.currJoy == 12) this.rivetsData.remappings.Joy_Mapping_Action_R = joyLast;
        }
        this.rivetsData.remapWait = false;
    }

    async setupLogin() {
        //prevent submit on enter 
        $('#txtPassword').bind("keypress", function (e) {
            if (e.keyCode == 13) {
                e.preventDefault();
                myClass.loginSubmit();
                return false;
            }
        });

        let pw = localStorage.getItem('n64wasm-password');
        if (pw==null)
            this.rivetsData.password = '';
        else
            this.rivetsData.password = pw;

        if (this.rivetsData.password){
            await this.loginSilent();
        }
            
    }

    loginModal(){
        $("#loginModal").modal();
        this.loginModalOpened = true;
        setTimeout(() => {
            //focus on textbox
            $("#txtPassword").focus();
        }, 500);
    }

    logout(){
        this.rivetsData.loggedIn = false;
        this.rivetsData.password = '';
        localStorage.setItem('n64wasm-password', this.rivetsData.password);
    }

    async loginSubmit(){
        $('#loginModal').modal('hide');
        this.loginModalOpened = false;
        let result = await this.loginToServer();
        if (result=='Success'){
            toastr.success('Logged In');
            localStorage.setItem('n64wasm-password', this.rivetsData.password);
            await this.getSaveStates();
            this.postLoginProcess();            
        }
        else{
            toastr.error('Login Failed');
            this.rivetsData.password = '';
            localStorage.setItem('n64wasm-password', '');
        }
    }

    async loginSilent(){
        if (!this.rivetsData.showLogin)
            return;
        
        let result = await this.loginToServer();
        if (result=='Success'){
            await this.getSaveStates();
            this.postLoginProcess();
        }
    }

    postLoginProcess(){
        //filter by .n64wasm extension and sort by date
        this.rivetsData.n64SaveStates = this.allSaveStates.filter((state)=>{
            return state.Name.endsWith('.n64wasm')
        });
        this.rivetsData.n64SaveStates.forEach(state => {
            state.Date = this.convertCSharpDateTime(state.Date);
        });
        this.rivetsData.n64SaveStates.sort((a,b)=>{ return b.Date.getTime() - a.Date.getTime() });
        this.rivetsData.loggedIn = true;
    }

    convertCSharpDateTime(initialDate) {
        let dateString = initialDate;
        dateString = dateString.substring(0, dateString.indexOf('T'));
        let timeString = initialDate.substr(initialDate.indexOf("T") + 1);
        let dateComponents = dateString.split('-');
        let timeComponents = timeString.split(':');
        let myDate = null;

        myDate = new Date(parseInt(dateComponents[0]), parseInt(dateComponents[1]) - 1, parseInt(dateComponents[2]),
            parseInt(timeComponents[0]), parseInt(timeComponents[1]), parseInt(timeComponents[2]));
        return myDate;
    }

    async loginToServer(){
        let result = await $.get(this.rivetsData.settings.CLOUDSAVEURL + '/Login?password=' + this.rivetsData.password);
        console.log('login result: ' + result);
        return result;
    }

    async getSaveStates(){
        let result = await $.get(this.rivetsData.settings.CLOUDSAVEURL + '/GetSaveStates?password=' + this.rivetsData.password);
        console.log('getSaveStates result: ', result);
        this.allSaveStates = result;
        result.forEach(element => {
            if (element.Name==this.rom_name + ".n64wasm")
                this.rivetsData.noCloudSave = false;
        });
        return result;
    }

    reset(){
        Module._neil_reset();
    }

    localCallback(){
    }
    
    
}
let myClass = new MyClass();
window["myApp"] = myClass; //so that I can reference from EM_ASM

//add any post loading logic to the window object
if (window.postLoad)
{
    window.postLoad();
}

window["Module"] = {
    onRuntimeInitialized: myClass.initModule,
    canvas: document.getElementById('canvas'),
    print: (text) => myClass.processPrintStatement(text),
    // printErr: (text) => myClass.print(text)
}

var rando2 = Math.floor(Math.random() * 100000);
var script2 = document.createElement('script');
script2.src = 'input_controller.js?v=' + rando2;
document.getElementsByTagName('head')[0].appendChild(script2);

