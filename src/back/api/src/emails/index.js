const path = require("path");
const nodemailer = require("nodemailer");
const Email = require("email-templates");

const config = require("../config");

const transport = nodemailer.createTransport({
  auth: {
    user: config.nodemailer.user,
    pass: config.nodemailer.password
  },
  host: "mail.privateemail.com",
  port: 587
});

const emailObject = new Email({
  views: {
    root: path.join(__dirname, "templates")
  },
  message: {
    from: config.nodemailer.user
  },
  transport
});

const sendVerify = async ({ email, username, code }) => {
  try {
    await emailObject.send({
      template: "verify",
      message: {
        to: email
      },
      locals: {
        username: username,
        code: code
      }
    });

    return true;
  } catch {
    return false;
  }
};

const sendVerifySuccess = async ({ email, username }) => {
  try {
    await emailObject.send({
      template: "verify-success",
      message: {
        to: email
      },
      locals: {
        username: username
      }
    });

    return true;
  } catch {
    return false;
  }
};

const sendRecovery = async ({ email, username, code }) => {
  try {
    await emailObject.send({
      template: "recovery",
      message: {
        to: email
      },
      locals: {
        username,
        email,
        code
      }
    });

    return true;
  } catch {
    return false;
  }
};

const sendRecoverySuccess = ({ email, username }) => {
  try {
    emailObject.send({
      template: "recovery-success",
      message: {
        to: email
      },
      locals: {
        username
      }
    });

    return true;
  } catch {
    return false;
  }
};

module.exports = {
  sendVerify,
  sendVerifySuccess,
  sendRecovery,
  sendRecoverySuccess
};
