const { gql } = require("apollo-server-express");

const resolvers = require("./resolvers");

const typeDefs = gql`
  extend type Query {
    me: User @auth
  }

  extend type Mutation {
    signup(username: String!, email: String!, password: String!): NotVerifedUser
    verify(username: String!, password: String!, code: String!): AuthData
    login(username: String!, password: String!): AuthData
    recovery(email: String!): Boolean

    changePassword(
      username: String!
      newPassword: String!
      code: String!
    ): AuthData
  }

  type User {
    id: ID!
    username: String!
    email: String!
  }

  type NotVerifedUser {
    id: ID!
    username: String!
    email: String!
    isVerified: Boolean!
  }

  type AuthData {
    user: User
    token: String!
  }
`;

module.exports = {
  typeDefs: [typeDefs],
  resolvers
};
