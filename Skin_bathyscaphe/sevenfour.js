var sevenfour = {};

sevenfour.scrollToNew = function () {
    var e = document.getElementById('new');
    window.scrollTo(0, e.offsetTop - 32);
};
sevenfour.scrollToMessage = function (number) {
    var e = document.getElementById(''+number);
    window.scrollTo(0, e.offsetTop - 32);
};

/* Image */
sevenfour.openImage = function (img) {
    document.location.href = img.src;
};
sevenfour.switchImageSize = function (img) {
    if (img.style.visibility == 'hidden')
        return;

    // thumbnail -> actual
    if (img.className == 'thumbnail') {
        img.className = 'actial';
    } else {
        img.className = 'thumbnail';
    }
};

sevenfour.extractImages = function(number) {
//    var dd = $('' + number).getElementsByTagName('dd')[0];
	var dd = document.getElementById('' + number).getElementsByTagName('dd')[0];
    
    if (dd.getElementsByTagName('img').length == 0) {
        $A(dd.getElementsByTagName('a')).each(function (a) {
                if (a.href.match(/^http:\/\/.+?(?:jpe?g|png|gif)$/)) {
                    var img = document.createElement('img');
                    img.src = a.href;
                    img.className = 'thumbnail';
                    img.onclick = function () {
                        if (img.className == 'actual')
                            img.className = 'thumbnail';
                        else
                            img.className = 'actual';
                    };
                    
                    dd.insertBefore(img, a);
                }
            });
    } else {
        /*
        $A(dd.getElementsByTagName('img')).each(function (img) {
                dd.removeChild(img);
            });
        */
    }
    return false;
};

sevenfour.countOfID = function (key) {
    return sevenfour.countOfIDs[key];
};
