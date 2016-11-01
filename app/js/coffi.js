phonon.options({
    navigator: {
        defaultPage: 'home',
        animatePages: true,
        enableBrowserBackButton: true,
        templateRootDirectory: 'tpl/'
    },
    i18n: null // for this example, we do not use internationalization
});

var app = phonon.navigator();

app.on({page: 'home', preventClose: false, content: null}, function(activity){
        activity.onTabChanged(function(tabnum){
            if (tabnum == "2" ){
                get_stats(stateStats);
                console.log("Changed to Info page");
            } if (tabnum == "3"){
                get_list();
            }
            else {
                console.log("Changed to Settings page");
            }

        });
        // Status of coffeemachine
        var coffeeStatus;

        // translate query results
        var statusHash = {
            ready: "Ready",
            off: "Off",
            flushing: "Is flushing",
            busy: "Is busy",
            need_water: "Needs water!!!",
            need_flushing: "Needs flushing!!!",
            full: "Trester is full",
            clean: "Needs cleaning",
            powder: "Fill in Powder"
        }

        activity.onCreate(function() {
            // localStorage.clear();

            // Initialize Coffee page buttons
            document.querySelector('.cmonecup').on('tap', onAction);
            document.querySelector('.cmon').on('tap', onAction);
            document.querySelector('.cmtwocups').on('tap', onAction);
            document.querySelector('.cmoff').on('tap', onAction);
            document.querySelector('.cmalarm').on('tap', onAction);
            document.querySelector('.cmflush').on('tap', onAction);

            document.querySelector('.cm-update-stats-icon').on('tap', onAction);

            // Add cm to localstorage
            document.getElementById('ipForm').addEventListener('submit', function() {
                var name = document.getElementById('name').value;
                var url = document.getElementById('ip').value;
                var coffeeMakers;
                if (localStorage.getItem("coffeemakers")== null) {
                    coffeeMakers = {};
                } else {
                    coffeeMakers = JSON.parse(localStorage.getItem("coffeemakers"));
                }
                coffeeMakers[name] = url;

                localStorage.setItem("coffeemakers", JSON.stringify(coffeeMakers));
                console.log(coffeeMakers);
                console.log(JSON.parse(localStorage.getItem('coffeemakers')));
                console.log("before");
                get_list();
                console.log("after");
            })




            // Query status every 2 seconds, unless page is hidden
            setInterval(function() {
                if (!document.hidden) {
                    getStatus();
                }
            }, 2000);
            get_stats(stateStats);
        });


        var alrt;
        var onAction = function(evt) {
            var target = evt.target;
            // action = 'ok';

            if(target.getAttribute('query') === 'onecup') {
                onecup(target);
            } 
            else if (target.getAttribute('query') === 'on') {
                turn_on(target);
            }
            else if (target.getAttribute('query') === 'off') {
                turn_off(target);
            } 
            else if (target.getAttribute('query') === 'twocups') {
                twocups(target);
            } 
            else if (target.getAttribute('query') === 'flush') {
                flush(target);
            } 
            else if (target.getAttribute('query') === 'update') {
                get_stats(stateStats);
            }
            else if (target.getAttribute('query') === 'alarm') {
                alarm(target);
            }
        };


    // Coffeemaker target functions
    // turn on coffeemaker
    function turn_on(target){
        if (coffeeStatus != "off") {
            phonon.alert("", "Coffeemaker is already on!!", true)
        }
        else {
            query = "command=on"
            function state(resultOn){
                alrt = phonon.indicator('Coffeemaker is starting', true);
                window.setTimeout(function(){
                    console.log(alrt);
                    alrt.close();
                    alrt = null;
                }, 2000);
            };
            query_coffeemaker(state, query);
        }
    }
    // Turn off coffeemaker
    function turn_off(target) {
        if (coffeeStatus == "off") {
            alrt = phonon.alert("","Coffeemaker is off", true, "");
            window.setTimeout(function(){
                alrt.close();
                alrt = null;
            }, 2000)

        }
        else {
            query = 'command=off';
            function state(resultOff){
                alrt = phonon.alert("", 'Turning off', true, "");
                window.setTimeout(function(){
                    console.log(alrt);
                    alrt.close()
                    alrt = null;
                }, 2000);
            };
            query_coffeemaker(state, query);
        }
    }

    // Make one cup
    function onecup(target) {
        if (coffeeStatus != "ready" && coffeeStatus != "clean") {
            alrt = phonon.alert(statusHash[coffeeStatus], "Coffeemaker is not ready",true, "");
            window.setTimeout(function(){
                console.log(alrt);
                alrt.close()
                alrt = null;
            }, 2000);

        }
        else {
            query = 'command=onecup'
            function state(resultOnecup){
                alrt = phonon.indicator('Making coffee', true);
                window.setTimeout(function(){
                    console.log(alrt);
                    alrt.close();
                    alrt = null;
                }, 2000);
            };
            query_coffeemaker(state, query);
        }
    }

    // Make two cups
    function twocups(target) {
        if (coffeeStatus != "ready" && coffeeStatus != "clean") {
            alrt = phonon.alert(statusHash[coffeeStatus], "Coffeemaker is not ready",true, "");
            window.setTimeout(function(){
                console.log(alrt);
                alrt.close()
                alrt = null;
            }, 2000);
        }
        else {

            query = 'command=twocups'
            function state(resultTwocups){
                alrt = phonon.indicator('Making 2 coffee', true);
                window.setTimeout(function(){
                    console.log(alrt);
                    alrt.close();
                    alrt = null;
                }, 2000);
            };
            query_coffeemaker(state, query);
        }
    }

    function flush(alrt) {
        if (coffeeStatus != "ready" && coffeeStatus != "need_flushing" && coffeeStatus != "clean") {
            alrt = phonon.alert("", "Coffeemaker is not ready for flushing");
            window.setTimeout(function(){
                console.log(alrt);
                alrt.close();
                alrt = null;
            }, 2000);

        }
        else {
            query = 'command=flush'
            function state(resultFlush){
                alrt = phonon.indicator('Flushing', true);
                window.setTimeout(function(){
                    console.log(alrt);
                    alrt.close();
                    alrt = null;
                }, 2000);
            };
            query_coffeemaker(state, query);
        }
    }

    function alarm(target) {
    }


    // Get total number of cups 
    function stateStats(resultStats){
        started = true;
        var today = resultStats["today"]
        var cmtoday = today["onecup"];
        cmtoday += today["double"];
        cmtoday +=  today["strong"];
        cmtoday += today["xstrong"];
        var week = resultStats["week"]
        var cmweek = week["onecup"];
        cmweek += week["double"];
        cmweek +=  week["strong"];
        cmweek += week["xstrong"];
        var total = resultStats["total"]
        var cmtotal = total["onecup"];
        cmtotal += total["double"];
        cmtotal +=  total["strong"];
        cmtotal += total["xstrong"];
        trester_progress = parseInt(resultStats["trester"]) / 640 * 100;
        console.log(resultStats["trester"]);


        // write list
        document.querySelector('.cm-update-stats-icon').innerHTML = "<i class=\"icon ion-android-sync big-icon cm-update-stats\" query=\"update\"></i>";
        document.querySelector('.cm-today').innerHTML = cmtoday;
        if (!today["onecup"] == 0 ){
        document.querySelector('.cm-single-today').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Normal</span> <span  class=\"pull-right\">" + today["onecup"].toString()+"</span> </li> ";
        }
        if (!today["double"] == 0 ){
        document.querySelector('.cm-double-today').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Double</span> <span  class=\"pull-right\">" + today["double"].toString()+"</span> </li> ";
        }
        if (!today["strong"] == 0 ){
        document.querySelector('.cm-strong-today').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Strong</span> <span  class=\"pull-right\">" + today["strong"].toString()+"</span> </li> ";
        }
        if (!today["xstrong"] == 0 ){
        document.querySelector('.cm-xstrong-today').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Xtra strong</span> <span  class=\"pull-right\">" + today["xstrong"].toString()+"</span> </li> ";
        }
        if (!today["flushs"] == 0 ){
        document.querySelector('.cm-flush-today').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Flushs</span> <span  class=\"pull-right\">" + today["flushs"].toString()+"</span> </li> ";
        }

        document.querySelector('.cm-week').innerHTML = cmweek;

        if (!week["onecup"] == 0) {
        document.querySelector('.cm-single-week').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Normal</span> <span  class=\"pull-right\">" + week["onecup"].toString()+"</span> </li> ";
        }
        if (!week["double"] == 0) {
        document.querySelector('.cm-double-week').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Double</span> <span  class=\"pull-right\">" + week["double"].toString()+"</span> </li> ";
        }
        if (!week["strong"] == 0) {
        document.querySelector('.cm-strong-week').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Strong</span> <span  class=\"pull-right\">" + week["strong"].toString()+"</span> </li> ";
        }
        if (!week["xstrong"] == 0) {
        document.querySelector('.cm-xstrong-week').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Xtra strong</span> <span  class=\"pull-right\">" + week["xstrong"].toString()+"</span> </li> ";
        }
        if (!week["flushs"] == 0) {
        document.querySelector('.cm-flush-week').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Flushs</span> <span  class=\"pull-right\">" + week["flushs"].toString()+"</span> </li> ";
        }
        document.querySelector('.cm-total').innerHTML =  cmtotal;
        document.querySelector('.cm-single-total').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Normal</span> <span  class=\"pull-right\">" + total["onecup"].toString()+"</span> </li> ";
        document.querySelector('.cm-double-total').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Double</span> <span  class=\"pull-right\">" + total["double"]+"</span> </li> ";
        document.querySelector('.cm-strong-total').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Strong</span> <span  class=\"pull-right\">" + total["strong"].toString()+"</span> </li> ";
        document.querySelector('.cm-xstrong-total').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Xtra strong</span> <span  class=\"pull-right\">" + total["xstrong"].toString()+"</span> </li> ";
        document.querySelector('.cm-flush-total').innerHTML = "<li class=\"padded-list\"> <span class=\"pull-left\">Flushs</span> <span  class=\"pull-right\">" + total["flushs"].toString()+"</span> </li> ";
        document.querySelector('.cm-trester-progress').innerHTML = "<div class=\"determinate\" style=\"width: "+trester_progress +"%;\"></div>";
    };


    // Read the cm status and insert into html
    function getStatus() {
        function statestatus(result){
            // console.log(result+"STATUS");
            switch (result) {
                case 'ready':
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-android-done status-icon"></i> Ready';
                    break;
                case 'clean':
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-android-sunny status-icon"></i> Need cleaning';
                    break;
                case "flushing":
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-waterdrop blink status-icon"></i> Flushing';
                    break;
                case "busy":
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-load-c status-icon icon-spin"></i> Busy';
                    break;
                case "need_water":
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-paintbucket blink status-icon warn-icon"></i> <span style="color: red;">Need water</span>';
                    break;
                case "need_flushing":
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-ios-rainy blink status-icon warn-icon"></i> <span style="color: red;">Need flushing</span>';
                    break;
                case 'full':
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-trash-a blink  status-icon warn-icon"></i> <span style="color: red;">Full</span>';
                    break;
                case "off":
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-power status-icon "></i> Off';
                    break;

                default:
                    document.querySelector('.cm-status').innerHTML = '<i class="icon ion-help status-icon warn-icon"></i> Unknown code: '+result;
                    break;
            };
            coffeeStatus =  result;
        };
        get_status(statestatus)
    }
});

// Query cm for cup statistics
function get_stats(statscallback) {
    document.querySelector('.cm-update-stats-icon').innerHTML = "<i class=\"icon ion-android-sync big-icon icon-spin cm-update-stats\" query=\"update\"></i>";
    console.log('Get stats');
    phonon.ajax({
        method: 'GET',
        url: localStorage.getItem('current')+'/stats',
        crossDomain: true,
        dataType: 'json',
        success: statscallback 
    });
}

// query actual cm status
function get_status(statuscallback) {
    phonon.ajax({
        method: 'GET',
        url: localStorage.getItem('current')+'/api?command=status',
        crossDomain: true,
        dataType: 'text',
        success: statuscallback 
    });
}

// Query cm api
function query_coffeemaker(callback, query) {
    console.log('Start con');
    phonon.ajax({
        method: 'GET',
        url: localStorage.getItem('current')+'/api?'+query,
        crossDomain: true,
        dataType: 'text',
        success: callback,
        error: function(){
            console.log(err);
        }
    });
}

// delete from cm list in localstorage
function deleteCm(evt) {
    var target = evt.target;
    console.log(target.getAttribute('delete'));
    coffeemakers = JSON.parse(localStorage.getItem('coffeemakers'));
    delete coffeemakers[target.getAttribute('delete')];
    localStorage.setItem('coffeemakers', JSON.stringify(coffeemakers));
    get_list();
}

// set current cm from list in localstorage
function setCurrentCm(evt) {
    var target = evt.target;
    currentcm = target.getAttribute('cmitem');
    console.log(currentcm);
    coffeemakers = JSON.parse(localStorage.getItem('coffeemakers'));
    console.log(coffeemakers[currentcm]);
    localStorage.setItem('current', coffeemakers[currentcm]);
    // console
    get_list();
}

// get list of cm in localstorage 
function get_list(){
    var coffeeMakersList = "";
    var cms = JSON.parse(localStorage.getItem('coffeemakers'))
    for (cm in cms){
        console.log(cm);
        console.log(cms[cm]);
        console.log(localStorage.getItem('current'));
         if (localStorage.getItem('current') == cms[cm]){
             coffeeMakersList += "<li class=\"padded-list\"> <div class=\"cm-list-item\"><span class=\"title\" cmitem=\""+cm+"\" >" + cm + "</span><span class=\"body\" cmitem=\""+cm+"\" >"+cms[cm] + "</span></div></li>"
     }
         else {
            coffeeMakersList += "<li> <a href=\"#action\" class=\"pull-right icon ion-close-circled cm-list-delete\" delete=\""+cm+"\"></a> <div class=\"padded-list cm-list-item\"><span class=\"title\" cmitem=\""+cm+"\">" + cm + "</span><span class=\"body\" cmitem=\""+cm+"\">"+cms[cm] + "</span></div></li>";

         }
    }
    document.querySelector(".cm-list").innerHTML = coffeeMakersList;

    cmItems = document.querySelectorAll('.cm-list-item');
    for ( elem in cmItems ) {
        cmItems[elem].onclick = setCurrentCm; 
    }
    cmItemsDel = document.querySelectorAll('.cm-list-delete');
    for ( elem in  cmItemsDel) {
        cmItemsDel[elem].onclick = deleteCm ;
                }
}


// Let's go!
app.start();
