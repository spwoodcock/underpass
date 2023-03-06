#!/usr/bin/python3
#
# Copyright (c) 2023 Humanitarian OpenStreetMap Team
#
# This file is part of Underpass.
#
#     Underpass is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
#
#     Underpass is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with Underpass.  If not, see <https://www.gnu.org/licenses/>.

#     This is a simple REST API that you can run easily to get data
#     from the Underpass database
#
#     1. Install requirements
#     pip install psycopg2 ; pip install fastapi ; pip install "uvicorn[standard]"
#
#     2. Run
#     uvicorn main:app --reload 
#
#     Optionally, for running the server into the Docker container
#     and access from the host:
#
#     uvicorn main:app --reload --host 0.0.0.0
#
#     3. Request
#     curl --location 'http://127.0.0.1:8000/report/dataQualityGeo' \
#       --header 'Content-Type: application/json' \
#       --data '{"fromDate": "2023-03-01T00:00:00"}'

import sys,os
sys.path.append(os.path.realpath('../dbapi'))

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from models import * 
from api import report
from api import db
import json

origins = [
    "http://localhost",
    "http://localhost:5000",
]

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

reporter = report.Report()
reporter.underpassDB = db.UnderpassDB("postgresql://underpass@postgis/galaxy")

@app.get("/")
def read_root():
    return {"Welcome": "This is the Underpass REST API."}

@app.post("/report/dataQualityGeo")
def dataQualityGeo(request: DataQualityRequest):
    if request.responseType:
        reporter.responseType = request.responseType
    results = reporter.getDataQualityGeo(
        fromDate = request.fromDate,
        toDate = request.toDate,
        hashtags = request.hashtags,
        area = request.area
    )
    return results

@app.post("/report/dataQualityTag")
def dataQualityTag(request: DataQualityRequest):
    if request.responseType:
        reporter.responseType = request.responseType
    results = reporter.getDataQualityTag(
        fromDate = request.fromDate,
        toDate = request.toDate,
        hashtags = request.hashtags,
        area = request.area
    )
    return results
try:
    import underpass as u

    @app.post("/osmchange/validate")
    def osmchangeValidate(request: OsmchangeValidateRequest):
        validator = u.Validate()
        return json.loads(validator.checkOsmChange(
            request.osmchange,
            request.check)
        )
except:
    pass