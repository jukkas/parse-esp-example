const Parse = require('parse/node');

Parse.initialize('YOUR-APPLICATION_ID-HERE',  // Change this. Application ID
                 'YOUR-PARSE-JS-API-KEY', // Back4app seems to want this. Set as '' if using self-hosted Parse server >= 3
                 'YOUR-MASTER_KEY-HERE'); // Master key
Parse.serverURL = 'https://parseapi.back4app.com/'; // Change this. Parse server address

async function signup_device1() {
    let user = new Parse.User();
    user.set('username', 'device1');
    user.set('password', 'demo');
    user.set('email', 'device1@example.com');

    // other fields can be set just like with Parse.Object
    user.set('foobar', 'Just testing');
    try {
        await user.signUp();
        console.log('User created!', user);
    } catch (error) {
        console.log('Error: ' + error.code + ' ' + error.message);
    }
}

async function signup_ui() {
    let user = new Parse.User();
    user.set('username', 'ui');
    user.set('password', 'demo');
    user.set('email', 'demo@example.com');

    try {
        await user.signUp();
        console.log('User demo created!', user);
    } catch (error) {
        console.log('Error: ' + error.code + ' ' + error.message);
    }
}

async function create_device1() {
    const Devices = Parse.Object.extend("Devices");
    const device = new Devices();

    device.save({
        name: 'device1',
        light: false,
        online: false,
        debug: false,
      })
      .then((newDevice) => {
        console.log('New device added');
        console.dir(newDevice);
      }, (error) => {
        console.error(error);
        // The save failed.
        // error is a Parse.Error with an error code and message.
        console.log(error);
      });
}

create_device1();
signup_device1();
signup_ui();

