# AWS API Gateway IP Rotator

- Rotates IPs and regions with each HTTP Request sent through AWS API Gateway
- Supports C++ with a C Wrapper

#### Usage

- Create API Gateway with secret and key ID in AWS

```
export AWS_ACCESS_KEY_ID=AK24235235235
export AWS_SECRET_ACCESS_KEY=fe0013442451337
```

#### Dependencies

- AWS SDK CPP // https://github.com/aws/aws-sdk-cpp

#### Examples

- app.cpp // C++
- wrapped.c // C
