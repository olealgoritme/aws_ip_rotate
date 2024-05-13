#!/bin/bash

# List all available AWS regions
regions=$(aws ec2 describe-regions --query "Regions[].RegionName" --output text)

# Loop through each region
for region in $regions; do
	echo "Checking region $region for API Gateways..."

	# List all REST API Gateways in the region
	apis=$(aws apigateway get-rest-apis --region $region --query "items[].id" --output text)

	# Loop through each API and delete
	for api in $apis; do
		echo "Deleting API Gateway $api in region $region..."
		aws apigateway delete-rest-api --region $region --rest-api-id $api
	done
done

echo "All API Gateways have been deleted across all regions."
