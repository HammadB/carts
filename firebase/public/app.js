var app = {};
var currentUID;
var deviceID;

app.signIn = function() {
    var provider = new firebase.auth.GoogleAuthProvider();
    firebase.auth().signInWithPopup(provider);
}

app.signOut = function() {
    firebase.auth().signOut();
}

app.registerDevice = function(e) {
    e.preventDefault();
    var device = parseInt($('#device_id').val());
    deviceID = device;
    var update = {};
    update['/devices/' + device] = true;
    update['/carts/' + device.toString()] = {"active": true};
    firebase.database().ref().update(update);
    $(this).hide();
    $('.shopping-cart-container').show();
    app.populateCart();
}

app.populateCallback = function(childSnapshot, prevChildKey){
    if (childSnapshot.key === 'active'){
        return;
    }
    $('table tbody')
    var prodRfid = childSnapshot.key;
    var quantity = childSnapshot.val();
    firebase.database().ref().child('products').child(prodRfid).on('value', function(dataSnapshot){
        if (dataSnapshot.val() !== null){
            var productDetails = dataSnapshot.val();
            var price = productDetails.price;
            var total = price * quantity;
            total = total.toFixed(2);
            var html = '<tr id="' + prodRfid + '"><th>' + productDetails.name + '</th><th>' + quantity + '</th><th>' + price + '</th><th class="total">' + total + '</th></tr>';
            var total;
            if ($('tr#' + prodRfid).length){
                $('tr#' + prodRfid).replaceWith(html);
                total = app.computeTotal();
                $('.total-cart').replaceWith('<tr class="total-cart"><th></th><th></th><th></th><th>' + total + '</th></tr>')
            } else{
                console.log('1');
                $('.total-cart').remove();
                console.log('2');
                $('table tbody').append(html);
                total = app.computeTotal();
                $('table tbody').append('<tr class="total-cart"><th></th><th></th><th></th><th>' + total + '</th></tr>')
            }
        }
    });
}

app.populateCart = function() {
    firebase.database().ref().child('carts').child(1234).on('child_changed', app.populateCallback);
    firebase.database().ref().child('carts').child(1234).on('child_added', app.populateCallback);
    firebase.database().ref().child('carts').child(1234).on('child_removed', app.removeItem);
}

app.removeItem = function(oldSnapshot) {
    var prodRfid = oldSnapshot.key;
    $('tr#' + prodRfid).remove();
    var total = app.computeTotal();
    $('.total-cart').replaceWith('<tr class="total-cart"><th></th><th></th><th></th><th>' + total + '</th></tr>')

}

app.handleAuthState = function(user) {
    if (user) {
        currentUID = user.uid;
        $('#signout').show();
        $('.form-inline').show();
        $('#signin-container').hide();
        
    } else {
        $('#signin-container').show();
        $('#signout').hide();
        $('.form-inline').hide();
        $('.shopping-cart-container').hide();
    }
}

app.computeTotal = function() {
    var total = 0;
    $('.total').each(function(i, val){
        total += parseFloat($(val).text());
    });
    total = total.toFixed(2);
    return total;
}

app.updateTotal = function(e) {
    var total = app.computeTotal();
    total = total * 100;
    $('.amount').val(total);
}

app.ready = function() {
    $('#signin').click(app.signIn);
    $('#signout').click(app.signOut);
    $('#register-device').submit(app.registerDevice);
    $('.stripe-button-el').click(app.updateTotal);
    firebase.auth().onAuthStateChanged(app.handleAuthState);

}

$(document).ready(app.ready);