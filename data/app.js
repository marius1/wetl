function ajax(url, callback, method, data) {
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function() {
        if (xmlHttp.readyState == XMLHttpRequest.DONE) {
            callback(xmlHttp.responseText);
        }
    };

    xmlHttp.open(method, url, true);
    xmlHttp.send(data);
}