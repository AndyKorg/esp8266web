function mouseover(c)
{
    c.style.textDecoration = "underline"
}
function mouseout(c)
{
    c.style.textDecoration = "none";
}
var sel = 1;
function child_cn(v)
{
    opt_sel(v);
    initDiv(v,"ru");
}
function opt_sel(v)
{
    document.getElementById("op_"+sel).className = "opt_no";
    document.getElementById("op_"+v).className = "opt_sel";
    sel = v;
}
function initDiv(v,lan)
{
    var child = document.getElementById("child_page");
    child.src = getUrl(v,lan);
	child_height(500);
}
function getUrl(v,lan)
{
    var end = ".html";
    switch(v)
    {
        case 1:return "clock"+end;break;//Часы
        case 2:return "alarm"+end;break;//Будильники
        case 3:return "sensor"+end;break;//Датчики
        case 4:return "ntptime"+end;break;//Время интернета
        case 5:return "param"+end;break;//Настройки
        case 6:return "wireless"+end;break;//Клиент WiFi
        case 7:return "wirepoint"+end;break;//Сервер WiFi
        case 8:return "restart"+end;break;//Рестарт
		case 9:return "reset"+end;break;//Возврат к заодским установкам
    }
}
function underline(c,v)
{
    if(v == 1)
    {
        c.style.textDecoration = "underline";
    }
    if(v == 2)
    {
        c.style.textDecoration = "none";
    }
}
function child_height(v)
{
	var child = document.getElementById("child_page");
	if(child != null)
	{
		child.style.height = v+"px";
	}
}

var setFormValues = function(form, cfg) {
	var name, field;
	for (name in cfg){
		if (form[name]) {
			field = form[name];
			if (field[1] && field[1].type === 'checkbox') {
				field = field[1];
			}
			if (field.type === 'checkbox'){
				field.checked = cfg[name] === '1' ? true : false;
			} else {
				field.value = cfg[name];
			}
		}
	}
}

var initMeniu = function() {
	var i, l = document.links;
	for (i = 0; i < l.length; i++) {
		if (l[i].href == document.URL) {
			l[i].className = 'active';
		}
	}
}
var $ = function(id) {
	return document.getElementById(id);
}
var reloadTimer = {
	s:10,
	reload:function(start) {
		if(start) {
			this.s = start;
		}
		$('timer').innerHTML = this.s < 10 ? '0' + this.s : this.s;
		if (this.s == 0){
			document.location.href = document.referrer != '' ? document.referrer : '/';
		}
		this.s--;
		setTimeout('reloadTimer.reload()', 1000);
	}
}

function goTest(){
	if (arguments.length > 1){
		var par, params = '', idElem = [];
		for(var i=0; i<arguments.length;i++){
			if (i != arguments.length-1)
				idElem[i] = arguments[i];
		}
		for(par in idElem){
			if (idElem[par])
				params += idElem[par]+'='+document.getElementById(idElem[par]).value;
			if (par != idElem.length-1)
				params += '&';
		}
		if (params.length){
			params += '&'+arguments[arguments.length-1]+'=1';
			var xhr = new XMLHttpRequest();
		//		xhr.open("POST", 'protect/submit.html', true)
			xhr.open("POST", '', true)
			xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded')
			xhr.send(params);
		}
	}
}

function SendSTOP(){
		var xhr = new XMLHttpRequest();
		xhr.open("POST", '', true)
		xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded')
		xhr.send("clock_stop=1");
}

function GetXMLRequest(url){
	if(window.XMLHttpRequest){
		var RetXML = new XMLHttpRequest();
		RetXML.open("GET", url, false);
		RetXML.send("");
	}
	else if(window.ActiveXObject){
			RetXML = new ActiveXObject("Microsoft.XMLDOM");
			RetXML.async = false;
			RetXML.load(url);
	}
	else{
		alert("Загрузка XML не поддерживается браузером");
		return null;
	}
	return RetXML;
}
function getXMLValue(xmlData, field) {
	try {
		xmlVal = xmlData.getElementsByTagName(field)[0];
		if (xmlVal.getElementsByTagName("value")[0].firstChild.nodeValue)
			return xmlVal.getElementsByTagName("value")[0].firstChild.nodeValue;
		else
			return null;
	} catch (err) {
		return null;
	}
}
