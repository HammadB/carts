var stripeToken = "sk_test_w7WHaI9Tbq3n8mrMybIjcWzA"
var stripe = require('stripe')(stripeToken)
var qs = require('querystring')

exports.handler = (event, context, callback) => {
	var resp;
	var parsedBody = qs.parse(event.body);

	var token = parsedBody.stripeToken;
	var chargeAmount = parsedBody.amount;

	console.log("Token is: " + token);

	var charge = stripe.charges.create({
	  amount: chargeAmount,
	  currency: "usd",
	  source: token,
	  description: "Example charge",
	}, function(err, charge) {
		console.log("Err is: " + err);
		console.log("charge is: " + charge);
	  	if (err) {
	    	test = { "statusCode": 400, "headers": { "Content-Type": "text/html"}, "body": "<html><body><h1>An Error Occured</h1></body></html>"};
		} else {
			test = { "statusCode": 200, "headers": { "Content-Type": "text/html"}, "body": '<html><head><meta name="viewport" content="width=device-width,initial-scale=1"><script src="https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js"></script><link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous"><script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js" integrity="sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa" crossorigin="anonymous"></script><link rel="stylesheet" href="style.css"><script src="https://use.fontawesome.com/1703a8fbc6.js"></script></head><body style="text-align:center;color:#fff;background:radial-gradient(circle,#F57C00,#FF5722)"><h1 style="margin-top:240px">Payment successful!</h1><i class="fa fa-check-circle fa-4x" aria-hidden="true"></i></body></html>'};
		}
	    callback(null, test);
	});
}